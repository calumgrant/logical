#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "EvaluationImpl.hpp"
#include "Database.hpp"
#include "OptimizerImpl.hpp"

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
    static std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Relation & node, bool parity, const std::shared_ptr<RecursiveLoop> & existingLoop)
    {
        if(node.loop && existingLoop)
            assert(node.loop == existingLoop);
        
        if(node.visited)
        {
            node.recursive = true;
            
            if(!node.loop)
                node.loop = existingLoop ? existingLoop : std::make_shared<RecursiveLoop>();
            
            if(node.parity != parity)
            {
                database.ParityError(node);
            }
            
            return node.loop;
        }
        
        if(node.analysedForRecursion)
            return node.loop;

        assert(!node.analysedForRecursion);
        node.visited = true;
        node.analysedForRecursion = true;
        node.parity = parity;
        bool hasRecursiveBranch = false;

        assert(!node.recursive);
        assert(!node.loop);

        node.VisitRules([&](Evaluation & eval)
        {
            auto l = AnalyseRecursion(database, eval, parity, node.loop);
            if( l )
                node.loop = l;
            
            if(eval.onRecursivePath) hasRecursiveBranch = true;
            if(node.loop && existingLoop)
                assert(node.loop == existingLoop);
        });
        
        // TODO: Scope-guard this
        node.visited = false;

        if(node.recursive)
        {
            assert(node.loop);
            assert(hasRecursiveBranch);
        }

        if(node.loop && existingLoop)
            assert(node.loop == existingLoop);
        return node.recursive ? nullptr : node.loop;
    }

    static std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Evaluation & node, bool parity, const std::shared_ptr<RecursiveLoop> & existingLoop)
    {
        std::shared_ptr<RecursiveLoop> loop = existingLoop;
        
        node.VisitReads([&](std::weak_ptr<Relation> & relation, int mask, const int*) {
            if((bool)(loop = AnalyseRecursion(database, *relation.lock(), parity, existingLoop)))
            {
                node.onRecursivePath = true;
                node.readIsRecursive = true;
            }
            if(existingLoop && loop)
                assert(loop == existingLoop);
        });
        
        node.VisitNext([&](std::shared_ptr<Evaluation>&n, bool nextIsNot) {
            if(auto l = AnalyseRecursion(database, *n, nextIsNot ? !parity : parity, loop))
            {
                node.onRecursivePath = true;
                assert( !loop || loop == l );
                loop = l;
            }
            if(existingLoop && loop)
                assert(loop == existingLoop);
        });
        
        // assert((bool)loop == node.onRecursivePath);
        
        if(existingLoop && loop)
            assert(loop == existingLoop);
            
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

    void AnalyseRecursion(Database & database, Relation & root) const
    {
        if(!root.analysedForRecursion)
            AnalyseRecursion(database, root, true, std::shared_ptr<RecursiveLoop>());
        
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
            if(!relation.recursive) return;
            
            relation.VisitRules([=](Evaluation&e) { CheckRule(e); });
            
            if(changed)
            {
                relation.VisitRules([=](const std::shared_ptr<Evaluation>&e) { OptimizeRule(e); });
                relation.SetRecursiveRules(baseRules, recursiveRules);
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
}
