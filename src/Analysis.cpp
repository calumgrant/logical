#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "Database.hpp"

#include <iostream>

std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &db, Relation & node, bool parity);
std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &db, Evaluation & node, bool parity);

void AnalyseRecursion(Database & db, Relation & root);


std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Relation & node, bool parity)
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

std::shared_ptr<RecursiveLoop> AnalyseRecursion(Database &database, Evaluation & node, bool parity)
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

void AnalyseRecursiveReads(Evaluation & node, bool depends)
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

void AnalyseRecursiveReads(Relation & relation)
{
    relation.VisitRules([&](Evaluation & eval) { AnalyseRecursiveReads(eval, false); });
}

void AnalyseRecursion(Database & database, Relation & root)
{
    if(!root.analysedForRecursion)
        AnalyseRecursion(database, root, true);
    
    AnalyseRecursiveReads(root);
}

void ApplyDeltas(Relation & relation)
{
    relation.VisitSteps([&](Evaluation&eval) {
        if(eval.readDelta)
            eval.useDelta = true;
    });
}

void AnalysePredicate(Database & database, Relation & root)
{
    if(root.analysed) return;
    root.analysed = true;
    
    AnalyseRecursion(database, root);
    
    if(database.Options().recursiveDeltas)
        ApplyDeltas(root);
}

OptimizationOptions CreateOptions(int level)
{
    OptimizationOptions options;
    
    switch(level)
    {
        case 0:
            options.recursiveDeltas = false;
            break;
    }
    
    return options;
}
