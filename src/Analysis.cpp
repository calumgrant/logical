#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "EvaluationImpl.hpp"
#include "Database.hpp"
#include "OptimizerImpl.hpp"

#include <iostream>

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
    std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Relation & node, bool parity) const
    {
        if(node.visited)
        {
            node.recursive = true;
            
            if(!node.loop)
                node.loop = std::make_shared<RecursiveLoop>();
            
            if(node.parity != parity)
            {
                database.ParityError(node);
            }
            
            return node.loop;
        }
        
        node.visited = true;
        node.analysedForRecursion = true;
        node.parity = parity;
        
        std::shared_ptr<RecursiveLoop> loop;
            
        node.VisitRules([&](Evaluation & eval)
        {
            // What happens if we get multiple recursive paths??
            auto l = AnalyseRecursion(database, eval, parity);
            if( l )
            {
                assert(!loop || l==loop);
                node.loop = l;
            }
        });
        
        // TODO: Scope-guard this
        node.visited = false;
            
        return node.recursive ? nullptr : loop;
    }

    std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Evaluation & node, bool parity) const
    {
        auto next1 = node.GetNext();
        auto next2 = node.GetNext2();
        
        auto relation = node.ReadsRelation();
        std::shared_ptr<RecursiveLoop> loop;
        
        if(relation)
        {
            if(!!(loop = AnalyseRecursion(database, *relation, parity)))
            {
                node.onRecursivePath = true;
                node.readIsRecursive = true;
            }
        }
        
        if(next1)
        {
            if(auto l = AnalyseRecursion(database, *next1, node.NextIsNot() ? !parity : parity))
            {
                node.onRecursivePath = true;
                assert( !loop || loop == l );
                loop = l;
            }
        }
        
        if(next2)
        {
            if(auto l = AnalyseRecursion(database, *next2, parity))
            {
                node.onRecursivePath = true;
                assert( !loop || loop == l );
                loop = l;
            }
        }
            
        return loop;
    }

    void AnalyseRecursiveReads(Evaluation & node, bool depends) const
    {
        node.dependsOnRecursiveRead = depends;
        if(!node.dependsOnRecursiveRead && node.readIsRecursive)
            node.readDelta = true;
        
        auto next1 = node.GetNext();
        auto next2 = node.GetNext2();
        
        if(next1)
        {
            AnalyseRecursiveReads(*next1, depends || node.readIsRecursive);
        }
        
        if(next2)
        {
            AnalyseRecursiveReads(*next2, depends || node.readIsRecursive);
        }
    }

    void AnalyseRecursiveReads(Relation & relation) const
    {
        relation.VisitRules([&](Evaluation & eval) { AnalyseRecursiveReads(eval, false); });
    }

    void AnalyseRecursion(Database & database, Relation & root) const
    {
        if(!root.analysedForRecursion)
            AnalyseRecursion(database, root, true);
        
        AnalyseRecursiveReads(root);
    }
};


void VisitEvaluation(Evaluation &e, const std::function<void(Evaluation&)> & fn)
{
    fn(e);
    if(auto next1 = e.GetNext())
    {
        fn(*next1);
        VisitEvaluation(*next1, fn);
    
        if(auto next2 = e.GetNext2())
        {
            fn(*next2);
            VisitEvaluation(*next2, fn);
        }
    }
}

void Relation::VisitSteps(const std::function<void(Evaluation&)> & fn) const
{
    VisitRules([&](Evaluation & rule) { VisitEvaluation(rule, fn); });
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
                auto orEval = dynamic_cast<OrEvaluation*>(re->GetNext());
                if(orEval)
                {
                    CheckOr(*re, *orEval);
                }
            }
        }
        
        void CheckOr(RuleEvaluation & rule, OrEvaluation &orEval)
        {
            if(orEval.onRecursivePath)
            {
                if(orEval.GetNext()->onRecursivePath)
                {
                    if(auto o = dynamic_cast<OrEvaluation*>(orEval.GetNext()))
                        CheckOr(rule, *o);
                }
                else
                {
                    changed = true;
                }
                if(orEval.GetNext2()->onRecursivePath)
                {
                    if(auto o = dynamic_cast<OrEvaluation*>(orEval.GetNext2()))
                        CheckOr(rule, *o);
                }
                else
                {
                    changed = true;
                }
            }
        }
        
        void OptimizeRule(const std::shared_ptr<Evaluation>&eval)
        {
            if(auto re = dynamic_cast<RuleEvaluation*>(&*eval))
            {
                if(re->onRecursivePath)
                {
                    auto next = re->GetNextPtr();
                    auto orEval = dynamic_cast<OrEvaluation*>(&*next);
                    if(orEval)
                    {
                        OptimizeEvaluationPath(eval, next);
                        return;
                    }
                }
            }
            
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
                    OptimizeEvaluationPath(ruleEval, orEval->GetNextPtr());
                    OptimizeEvaluationPath(ruleEval, orEval->GetNext2Ptr());
                }
                else
                {
                    if(ruleEval->GetNextPtr() == eval)
                        AddRecursiveRule(ruleEval);
                    else
                        AddRecursiveRule(CreateRule(ruleEval, eval));
                }
            }
            else
            {
                if(ruleEval->GetNextPtr() == eval)
                    AddBaseRule(ruleEval);
                else
                    AddBaseRule(CreateRule(ruleEval, eval));
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
        relation.VisitSteps([&](Evaluation&eval) {
            if(eval.readDelta)
                eval.useDelta = true;
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
    static Recursion recursion;
    static Deltas deltas;
    static RecursiveBranch recursiveBranch;
    
    RegisterOptimization(recursion);
    RegisterOptimization(deltas);
    RegisterOptimization(recursiveBranch);
}
