#include "Database.hpp"
#include "Evaluation.hpp"

#include <iostream>

void UnaryTable::Add(const Entity *row)
{
    MakeDirty();
    //std::cout << "Added (" << (int)e.type << "," << e.i << ") to the table\n";
    values.insert(row[0]);
}

void BinaryTable::Add(const Entity * row)
{
    MakeDirty();
    //std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
    values.insert(std::make_pair(row[0],row[1]));
}

int UnaryTable::Count()
{
    RunRules();
    return values.size();
}

int BinaryTable::Count()
{
    RunRules();
    return values.size();
}

PrintRelation::PrintRelation(Database &db) : database(db)
{
}

void PrintRelation::Add(const Entity * row)
{
    database.Print(row[0], std::cout);
}

int PrintRelation::Count()
{
    return 0;
}

int TableX::Count()
{
    return 0;
}

void UnaryTable::Query(Entity * row, Visitor &v)
{
    RunRules();
    for(auto &i : values)
    {
        v.OnRow(&i);
    }
}

void PrintRelation::Query(Entity * row, Visitor&)
{
    // Empty relation.
}

void BinaryTable::Query(Entity * row, Visitor&v)
{
    RunRules();
    // todo
}

void TableX::Query(Entity * row, Visitor&v)
{
    // todo
}

void TableX::Add(const Entity *row)
{
}

Predicate::Predicate() : rulesRun(false)
{
}

void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    rules.push_back(rule);
    MakeDirty();
}

void Predicate::MakeDirty()
{
    rulesRun = false;
}

void Predicate::RunRules()
{
    if(rulesRun) return;
    
    for(auto p : rules)
        p->Evaluate(nullptr);
    
    rulesRun = true;
}
