#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "Database.hpp"

#include <iostream>



Relation * AnalyseRecursion(Database &db, Relation & node, bool parity);
Relation * AnalyseRecursion(Database &db, Evaluation & node, bool parity);

void AnalyseRecursion(Database & db, Relation & root);


Relation * AnalyseRecursion(Database &database, Relation & node, bool parity)
{
    if(node.visited)
    {
        node.recursive = true;
        node.onRecursivePath = true;
        node.recursiveRoot = &node;
        
        if(node.parity != parity)
        {
            database.ParityError(node);
        }
        
        return &node;
    }
    
    node.visited = true;
    node.analysedForRecursion = true;
    node.parity = parity;
    
    Relation * root = nullptr;
        
    node.VisitRules([&](Evaluation & eval)
    {
        // What happens if we get multiple recursive paths??
        auto r = AnalyseRecursion(database, eval, parity);
        if( r )
        {
            assert(!root || r==root);
            node.onRecursivePath = true;
            node.recursiveRoot = r;
        }
    });
    
    // TODO: Scope-guard this
    node.visited = false;
    
    assert(!node.recursive || node.recursiveRoot == &node);
    
    return node.recursive ? nullptr : root;
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

Relation * AnalyseRecursion(Database &database, Evaluation & node, bool parity)
{
    auto next1 = node.GetNext();
    auto next2 = node.GetNext2();
    
    auto relation = node.ReadsRelation();
    Relation * root = nullptr;
    
    if(relation)
    {
        if(!!(root = AnalyseRecursion(database, *relation, parity)))
        {
            node.onRecursivePath = true;
            node.readIsRecursive = true;
        }
    }
    
    if(next1)
    {
        if(auto r = AnalyseRecursion(database, *next1, node.NextIsNot() ? !parity : parity))
        {
            node.onRecursivePath = true;
            assert( !root || root == r );
            root = r;
        }
    }
    
    if(next2)
    {
        if(auto r = AnalyseRecursion(database, *next2, parity))
        {
            node.onRecursivePath = true;
            assert( !root || root == r );
            root = r;
        }
    }
        
    return root;
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
