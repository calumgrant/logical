#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "Database.hpp"

#include <iostream>

bool AnalyseRecursion(Database &database, Relation & node, bool parity)
{
    if(node.visited)
    {
        node.recursive = true;
        node.onRecursivePath = true;
        
        if(node.parity != parity)
        {
            database.ParityError(node);
        }
        
        return true;
    }
    
    node.visited = true;
    node.analysedForRecursion = true;
    node.parity = parity;
        
    node.VisitRules([&](Evaluation & eval)
    {
        if( AnalyseRecursion(database, eval, parity) )
            node.onRecursivePath = true;
    });
    
    // TODO: Scope-guard this
    node.visited = false;
    
    return node.onRecursivePath && !node.recursive;
}

bool AnalyseRecursion(Database &database, Evaluation & node, bool parity)
{
    auto next1 = node.GetNext();
    auto next2 = node.GetNext2();
    
    auto relation = node.ReadsRelation();

    bool onRecursivePath = false;
    
    if(relation)
    {
        if(AnalyseRecursion(database, *relation, parity))
            onRecursivePath = true;
    }
    
    if(next1)
    {
        if(AnalyseRecursion(database, *next1, node.NextIsNot() ? !parity : parity))
            onRecursivePath = true;
    }
    
    if(next2)
    {
        if(AnalyseRecursion(database, *next2, parity))
            onRecursivePath = true;
    }
    
    return onRecursivePath;
}

void AnalyseRecursion(Database & database, Relation & root)
{
    if(root.analysedForRecursion) return;
    
    AnalyseRecursion(database, root, true);
}
