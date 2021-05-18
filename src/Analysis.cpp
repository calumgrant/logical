#include "Analysis.hpp"
#include "Evaluation.hpp"
#include "Database.hpp"

#include <iostream>

bool AnalyseRecursion(Database &database, Relation & node, Parity parity)
{
    if(node.visited)
    {
        std::cout << "The recursive predicate is ";
        Evaluation::OutputRelation(std::cout, database, node);
        std::cout << std::endl;

        node.recursive = true;
        node.onRecursivePath = true;
        return true;
    }
    
    node.visited = true;
    node.analysedForRecursion = true;
        
    node.VisitRules([&](Evaluation & eval)
    {
        if( AnalyseRecursion(database, eval, parity) )
            node.onRecursivePath = true;
    });
    
    // TODO: Scope-guard this
    node.visited = false;
    
    return node.onRecursivePath && !node.recursive;
}

bool AnalyseRecursion(Database &database, Evaluation & node, Parity parity)
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
        if(AnalyseRecursion(database, *next1, parity))
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
    
    AnalyseRecursion(database, root, Parity::Even);
}
