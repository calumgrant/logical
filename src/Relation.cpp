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

PrintRelation::PrintRelation(std::ostream & output, Database &db) :
    Predicate("print"), output(output), database(db)
{
}

void PrintRelation::Add(const Entity * row)
{
    database.Print(row[0], output);
    output << std::endl;
}

void PrintRelation::AddRule(const std::shared_ptr<Evaluation> & eval)
{
    // Run the rule immediately.
    eval->Evaluate(nullptr);
}

int PrintRelation::Count()
{
    return 0;
}

ErrorRelation::ErrorRelation(Database &db) : PrintRelation(std::cout, db)
{
}

void ErrorRelation::Add(const Entity * row)
{
    PrintRelation::Add(row);
    database.ReportUserError();
}

int TableX::Count()
{
    return 0;
}

void UnaryTable::Query(Entity * row, int columns, Visitor &v)
{
    RunRules();
    
    switch(columns)
    {
        case 0:
            for(auto &i : values)
            {
                v.OnRow(&i);
            }
            break;
        case 1:
            {
                auto i = values.find(row[0]);
                if (i != values.end())
                    v.OnRow(row);
                break;
            }
            break;
        default:
            assert(!"Invalid query columns");
    }
}

void PrintRelation::Query(Entity * row, int, Visitor&)
{
    // Empty relation.
}

void BinaryTable::Query(Entity * row, int bound, Visitor&v)
{
    RunRules();
    
    switch(bound)
    {
        case 0:
            for(auto & i : values)
            {
                Entity data[2];
                // TODO: Store a pair in the table to make the scan faster
                data[0] = i.first;
                data[1] = i.second;
                v.OnRow(data);
            }
        default:
            // Not implemented yet
            break;
    }
    // todo
}

void TableX::Query(Entity * row, int, Visitor&v)
{
    // todo
}

void TableX::Add(const Entity *row)
{
}

Predicate::Predicate(const std::string &name) : rulesRun(false), name(name)
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
    
    for(auto & p : rules)
        p->Evaluate(nullptr);
    
    rulesRun = true;
}

UnaryTable::UnaryTable(const std::string &name) : Predicate(name)
{
}

BinaryTable::BinaryTable(const std::string &name) : Predicate(name)
{
}

TableX::TableX(const std::string &name) : Predicate(name)
{
}

const std::string & Predicate::Name() const
{
    return name;
}
