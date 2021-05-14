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
    
    auto value = std::make_pair(row[0],row[1]);
    //std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
    auto p = values.insert(value);
    
    if(p.second)
    {
        // TODO: Lazy map initialization
        map1.insert(value);
        map2.insert(std::make_pair(row[1], row[0]));
    }
}

std::size_t UnaryTable::Count()
{
    return values.size();
}

std::size_t BinaryTable::Count()
{
    return values.size();
}

PrintRelation::PrintRelation(std::ostream & output, Database &db, int name) :
    Predicate(db, name), output(output)
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
    if( database.Explain() )
        eval->Explain(database, std::cout, 0);
}

std::size_t PrintRelation::Count()
{
    return 0;
}

ErrorRelation::ErrorRelation(Database &db) : PrintRelation(std::cout, db, db.GetStringId("error"))
{
}

void ErrorRelation::Add(const Entity * row)
{
    PrintRelation::Add(row);
    database.ReportUserError();
}

std::size_t Table::Count()
{
    return hash.size();
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
            break;
        case 1:
            {
                auto range = map1.equal_range(row[0]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[1] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 2:
            {
                auto range = map2.equal_range(row[1]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[0] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 3:
            {
                if(values.find(std::make_pair(row[0], row[1])) != values.end())
                    v.OnRow(row);
            }
            break;
        default:
            std::cout << "TODO: Implement the join\n";
            // Not implemented yet
            break;
    }
    // todo
}

Table::map_type & Table::GetIndex(int mask)
{
    auto i = indexes.find(mask);
    if(i != indexes.end())
    {
        return i->second;
    }
    
    // Create an index
    
    map_type it({}, 100, Comparer(data, arity, mask), Comparer(data, arity, mask));
    
    auto &index = indexes.insert(std::make_pair(mask, it)).first->second;
    
    for(auto i=0; i<data.size(); i+=arity)
    {
        index.insert(i);
    }
    
    return index;
}

void Table::Query(Entity * row, int mask, Visitor&v)
{
    RunRules();
    
    if(mask==0)
    {
        for(int s=0; s<data.size(); s+=arity)
            v.OnRow(&data[s]);
        return;
    }
    
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    if(mask == -1)
    {
        auto i = hash.find(s);
        data.resize(s);
        
        if (i != hash.end())
            v.OnRow(&data[*i]);
        return;
    }
    else
    {
        auto result = GetIndex(mask).equal_range(s);
        data.resize(s);

        for(auto i = result.first; i!=result.second; ++i)
        {
            v.OnRow(&data[*i]);
        }
    }
}

void Table::Add(const Entity *row)
{
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    auto i = hash.insert(s);
    if(!i.second)
    {
        // It was duplicated, so remove it.
        data.resize(s);
    }
    
    // Insert into all other indexes
    for(auto & index : indexes)
    {
        index.second.insert(s);
    }
}

Predicate::Predicate(Database &db, int name) :
    rulesRun(false), name(name), database(db),
    evaluating(false), recursive(false), sizeAtLastRecursiveCall(0)
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
    
    if(evaluating)
    {
        if(!recursive)
        {
            recursive = true;
            sizeAtLastRecursiveCall = Count();
        }
        return;
    }
        
    evaluating = true;
    
    std::size_t iteration = 1;
    
    if(database.Explain())
        std::cout << "Evaluating " << database.GetString(Name()) << std::endl;

    do
    {
        sizeAtLastRecursiveCall = Count();
        recursive = false;
        for(auto & p : rules)
        {
            p->Evaluate(nullptr);
            if(database.Explain())
            {
                p->Explain(database, std::cout, 0);
            }
        }
        ++iteration;
    }
    while (recursive && Count()>sizeAtLastRecursiveCall);
    
    evaluating = false;
    rulesRun = true;
}

UnaryTable::UnaryTable(Database &db, int name) : Predicate(db, name)
{
}

BinaryTable::BinaryTable(Database &db, int name) : Predicate(db, name)
{
}

Table::Table(Database &db, int name, int arity) : Predicate(db, name), arity(arity), hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1))
{
}

int Predicate::Name() const
{
    return name;
}

Table::Comparer::Comparer(const std::vector<Entity> & base, int arity, int mask) : base(base), arity(arity), mask(mask)
{    
}

std::size_t Relation::GetCount()
{
    RunRules();
    return Count();
}
