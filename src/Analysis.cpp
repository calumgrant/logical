#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "EvaluationImpl.hpp"
#include "Database.hpp"
#include "OptimizerImpl.hpp"
#include "RelationImpl.hpp"

#include <iostream>


class BindingAnalysis : public Optimization
{
public:
    BindingAnalysis() : Optimization("semi-naive", "Implements semi-naive binding", 1)
    {
    }
    
    void Analyse(Relation & relation) const override
    {
        relation.VisitSteps([](EvaluationPtr&eval) {
            eval->VisitReads([&](std::weak_ptr<Relation> & rel, int mask, const int * inputs) {
                if(mask != 0)
                {
                    std::cout << "Semi-naive opportunity: " << mask << "\n";

                    auto relation = rel.lock();
                    if(!relation->IsSpecial())
                    {
                        auto guard = relation->GetBindingRelation(mask);
                        auto bound = relation->GetBoundRelation(mask);
                        
                        std::vector<int> writes;
                        for(int i=0; i<relation->Arity(); ++i)
                            if(inputs[i] != -1) writes.push_back(i);
                        
                        auto write = std::make_shared<Writer>(guard, writes);
                        eval = std::make_shared<OrEvaluation>(write, eval);
                        rel = bound;
                    }
                }
            });
        });
        
        relation.VisitRules([](Evaluation &eval) {
            
        });
        
    }
};


class Recursion : public Optimization
{
public:
    Recursion() : Optimization("recursion", "Analyses recursion (non-optional optimization)", 0)
    {
    }
    
    void Analyse(Relation & relation) const
    {
        AnalyseRecursion(relation.GetDatabase(), relation);
    }
    
private:
    
    static Relation * GetLoop(Relation * r)
    {
        return r ? &GetLoop(*r) : nullptr;
    }

    static Relation & GetLoop(Relation & r)
    {
        auto *rel = &r;
        while(auto n = rel->backEdge)
            rel = n;
        return *rel;
    }

    static Relation * MergeLoops(Relation * r1, Relation * r2)
    {
        if(!r1) return r2;
        if(!r2) return r1;
        while(auto n = r1->backEdge)
            r1 = n;
        while(auto n = r2->backEdge)
            r2 = n;
        if(r1==r2) return r1;
        assert(r1->recursiveDepth != r2->recursiveDepth);
        if(r1->recursiveDepth < r2->recursiveDepth)
        {
            r2->backEdge = r1;
            return r1;
        }
        else
        {
            r1->backEdge = r2;
            return r2;
        }
    }
    
    // Returns a back edge if in a loop, or nullptr if this branch is not recursive
    static Relation * AnalyseRecursion(Database &database, Relation & node, bool parity, int depth, int root)
    {
        if(node.recursiveDepth>=0)
        {
            // This node has been analysed previously.
            // Either in the same analysis, or in a previous analysis.
            
            if(node.recursiveRoot == root)
            {
                // A forward edge
                if(node.recursiveDepth > depth)
                    return nullptr;
                
                // This is recursive, so we return this node as a back-edge.

                return GetLoop(&node);
            }
            
            // We have reached a node that was analysed in a previous cycle.
            // (note we have incremental analysis).
            return nullptr;
        }
        
        node.recursiveDepth = depth;
        node.parity = parity;
        
        node.recursiveRoot = root;
        node.analysedForRecursion = true;
        
        Relation * loop = nullptr;

        node.VisitRules([&](Evaluation & eval)
        {
            auto l = AnalyseRecursion(database, eval, parity, depth+1, root);
            loop = MergeLoops(loop, l);
        });
        
        if(!loop) return nullptr;
        
        if(loop == & node)
        {
            node.inRecursiveLoop = true;
            
            if(node.parity != parity)
            {
                database.ParityError(node);
            }
            
            return nullptr;
        }
        
        if(loop->recursiveDepth < node.recursiveDepth)
        {
            node.backEdge = loop;
            return loop;
        }
        
        return nullptr;
    }

    // Same return as previous function
    // Returns a back-edge
    // Returns the outermost back-edge by looking at the recursive Depth.
    static Relation * AnalyseRecursion(Database &database, Evaluation & node, bool parity, int depth, int root)
    {
        Relation * loop = nullptr;
        
        node.VisitReads([&](std::weak_ptr<Relation> & relation, int mask, const int*) {
            loop = MergeLoops(loop, AnalyseRecursion(database, *relation.lock(), parity, depth+1, root));
        });
        
        node.VisitNext([&](std::shared_ptr<Evaluation>&n, bool nextIsNot) {
            loop = MergeLoops(loop, AnalyseRecursion(database, *n, nextIsNot ? !parity : parity, depth+1, root));
        });

        if(loop)
            node.onRecursivePath = true;
        return loop;
    }

    void AnalyseRecursiveReads(Evaluation & node, bool depends) const
    {
        node.dependsOnRecursiveRead = depends;
        if(!node.dependsOnRecursiveRead && node.readIsRecursive)
            node.readDelta = true;
        
        node.VisitNext([&](EvaluationPtr & next, bool)
        {
            AnalyseRecursiveReads(*next, depends || node.readIsRecursive);
        });
    }

    void AnalyseRecursiveReads(Relation & relation) const
    {
        relation.VisitRules([&](Evaluation & eval) { AnalyseRecursiveReads(eval, false); });
    }
    
    mutable int rootCount = 0;

    void AnalyseRecursion(Database & database, Relation & root) const
    {        
        if(!root.analysedForRecursion)
        {
            AnalyseRecursion(database, root, true, 1, rootCount++);
            assert(root.analysedForRecursion);
            // assert(root.recursiveRoot == root);
        }

        if(!root.loop)
        {
            auto & recursiveLoop = GetLoop(root);
            
            // assert(&root == &recursiveLoop);  // Deleteme - debugging only
            
            if(!recursiveLoop.loop)
            {
                auto loop = std::make_shared<ExecutionUnit>(database);
                recursiveLoop.loop = loop;
                loop->AddRelation(recursiveLoop);
            }
            
            if(!root.loop)
            {
                root.loop = recursiveLoop.loop;
                root.loop->AddRelation(root);
            }
        }
        
        AnalyseRecursiveReads(root);
    }
};


void VisitEvaluation(EvaluationPtr &e, const std::function<void(EvaluationPtr&)> & fn)
{
    auto p = e;

    fn(e);

    // Visit p, not e. Don't re-visit anything that fn creates.
    p->VisitNext([&](EvaluationPtr &n, bool) {
        VisitEvaluation(n, fn);
    });
    
}

void Relation::VisitSteps(const std::function<void(EvaluationPtr&)> & fn)
{
    VisitRules([&](EvaluationPtr & rule) { VisitEvaluation(rule, fn); });
}



Optimization::Optimization(const char * name, const char * description, int level) : name(name), description(description), level(level)
{
}

/*
 This optimization identifies those rules that form a base case and those rules that form a recursive case.
 In the simplistic case (implemented here), we simply look for "or"
 */
class RecursiveBranch : public Optimization
{
public:
    RecursiveBranch() :
        Optimization("recursive-branch", "Only evaluate base cases once in a recursive predicate.", 1)
    {
    }

    class BranchImpl
    {
    public:
        BranchImpl(Database &database, Relation &rel) :
            relation(rel)
        {
            if(!relation.inRecursiveLoop) return;
            
            relation.VisitRules([=](Evaluation&e) { CheckRule(e); });
            
            if(changed)
            {
                relation.VisitRules([=](const std::shared_ptr<Evaluation>&e) { OptimizeRule(e); });
                relation.loop->rules.SetRecursiveRules(baseRules, recursiveRules);
            }
        }
        
        void CheckRule(Evaluation & eval)
        {
            if(!eval.onRecursivePath) return;
            
            if(auto re = dynamic_cast<RuleEvaluation*>(&eval))
            {
                re->VisitNext([&](EvaluationPtr & next, bool) {
                    if(auto orEval = dynamic_cast<OrEvaluation*>(&*next))
                        CheckOr(*re, *orEval);
                });
            }
        }
        
        void CheckOr(RuleEvaluation & rule, OrEvaluation &orEval)
        {
            if(orEval.onRecursivePath)
            {
                orEval.VisitNext([&](EvaluationPtr & next, bool) {
                    if(next->onRecursivePath)
                    {
                        if(auto o = dynamic_cast<OrEvaluation*>(&*next))
                            CheckOr(rule, *o);
                    }
                    else
                    {
                        changed = true;
                    }
                });
            }
        }
        
        void OptimizeRule(const std::shared_ptr<Evaluation>&eval)
        {
            bool addBaseRule = true;
            if(auto re = dynamic_cast<RuleEvaluation*>(&*eval))
            {
                if(re->onRecursivePath)
                {
                    re->VisitNext([&](EvaluationPtr &next, bool) {
                        if( dynamic_cast<OrEvaluation*>(&*next) )
                    {
                        OptimizeEvaluationPath(eval, next);
                            addBaseRule = false;
                    }
                    });
                }
            }
            
            if(addBaseRule)
                AddBaseRule(eval);
        }
        
        std::shared_ptr<Evaluation> CreateRule(const std::shared_ptr<Evaluation> & ruleEval, const std::shared_ptr<Evaluation> & branch)
        {
            return ruleEval->WithNext(branch);
        }
        
        void OptimizeEvaluationPath(const std::shared_ptr<Evaluation> & ruleEval, const std::shared_ptr<Evaluation> & eval)
        {
            if(eval->onRecursivePath)
            {
                auto orEval = dynamic_cast<OrEvaluation*>(&*eval);
                if(orEval)
                {
                    orEval->VisitNext([&](EvaluationPtr & next, bool) {
                        OptimizeEvaluationPath(ruleEval, next);
                    });
                }
                else
                {
                    ruleEval->VisitNext([&](EvaluationPtr & next, bool) {
                        if( next == eval )
                            AddRecursiveRule(ruleEval);
                        else
                            AddRecursiveRule(CreateRule(ruleEval, eval));
                    });
                }
            }
            else
            {
                ruleEval->VisitNext([&](EvaluationPtr & next, bool) {
                    if( next == eval )
                        AddBaseRule(ruleEval);
                    else
                        AddBaseRule(CreateRule(ruleEval, eval));
                });
            }
        }
            
        void AddBaseRule(const std::shared_ptr<Evaluation>&eval)
        {
            baseRules = baseRules ? std::make_shared<OrEvaluation>(baseRules, eval) : eval;
        }
        
        void AddRecursiveRule(const std::shared_ptr<Evaluation>&eval)
        {
            recursiveRules = recursiveRules ? std::make_shared<OrEvaluation>(recursiveRules, eval) : eval;
        }
        
        bool changed = false;
        
        Relation & relation;
        
        std::shared_ptr<Evaluation> baseRules, recursiveRules;

    };
    
    void Analyse(Relation & relation) const override
    {
        BranchImpl(relation.GetDatabase(), relation);
    }
};

class Deltas : public Optimization
{
public:
    Deltas() : Optimization("deltas", "Query the delta of the previous iteration when it is safe to do so.", 1)
    {
    }
    
    void Analyse(Relation & relation) const override
    {
        relation.VisitSteps([&](EvaluationPtr&eval) {
            if(eval->readDelta)
                eval->useDelta = true;
        });
    }

};

void AnalysePredicate(Database & database, Relation & root)
{
    if(root.analysed) return;
    root.analysed = true;
    database.GetOptimizer().Optimize(root);
}

void OptimizerImpl::Visit(const std::function<void(Optimization&)> &v)
{
    for(auto &i : allOptimizations)
        v(*i);
}

void Optimizer::SetLevel(int level)
{
    Visit([=](Optimization & opt) {
        opt.enabled = opt.level==0 || opt.level <= level;
    });
    UpdateActiveList();
}

void OptimizerImpl::RegisterOptimization(Optimization &opt)
{
    allOptimizations.push_back(&opt);
    UpdateActiveList();
}

void OptimizerImpl::UpdateActiveList()
{
    std::vector<Optimization*> list;
    list.reserve(allOptimizations.size());
    for(auto &i : allOptimizations)
        if(i->enabled) list.push_back(i);
    activeOptimizations = std::move(list);
}

void OptimizerImpl::Optimize(Relation & relation) const
{
    for(auto o : activeOptimizations)
        o->Analyse(relation);
}



OptimizerImpl::OptimizerImpl()
{
    static BindingAnalysis binding;
    static Recursion recursion;
    static Deltas deltas;
    static RecursiveBranch recursiveBranch;
    
    RegisterOptimization(binding);
    RegisterOptimization(recursion);
    RegisterOptimization(deltas);
    RegisterOptimization(recursiveBranch);
    
    SetLevel(0);
}
