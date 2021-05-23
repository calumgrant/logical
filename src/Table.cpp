#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"

#include <iostream>

Table::Table(Database &db, const CompoundName & cn, int arity) :
    Predicate(db, cn.parts[0]), arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1)),
    name(cn)
{
}

Table::Table(Database &db, RelationId name, int arity) :
    Predicate(db, name), arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1)),
    name()
{
    
}

void Table::Add(const Entity *row)
{
    /*
    std::cout << "Writing (";
    for(int i=0; i<arity; ++i)
    {
        if(i>0) std::cout << ", ";
        database.PrintQuoted(row[i], std::cout);
    }
    std::cout << ") into " << database.GetString(Name()) << std::endl;
     */
    
    for(int i=0; i<arity; ++i)
        assert(row[i].type != EntityType::None);
    
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    auto i = hash.insert(s);
    if(!i.second)
    {
        // It was duplicated, so remove it.
        data.resize(s);
    }
}

bool Table::NextIteration()
{
    bool moreResults = deltaEnd < data.size();
    
    for(std::size_t s = deltaStart; s < deltaEnd; s+= arity)
    {
        for(auto & index : indexes)
        {
            index.second.insert(s);
        }
    }
    deltaStart = deltaEnd;
    deltaEnd = data.size();
    return moreResults;
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
    
    for(auto i=0; i<deltaStart; i+=arity)
    {
        index.insert(i);
    }
    
    return index;
}

void Table::Query(Entity * row, int mask, Visitor&v)
{
    if(!HasRules() && data.empty())
    {
        database.WarningEmptyRelation(*this);
        return;
    }
    
    RunRules();
    
    if(mask==0)
    {
        // Unclear if we need reentrancy guard here.
        auto r = reentrancy.Enter();
        for(std::size_t s=0; s<data.size(); s+=arity)
            v.OnRow(&data[s]);
    }
    else
    {
        
        if(mask == -1 || mask == (1<<arity)-1)
        {
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto i = hash.find(s);
            data.resize(s);
            
            if (i != hash.end() && *i<deltaStart)
                v.OnRow(&data[*i]);
            return;
        }
        else
        {
            auto & index = GetIndex(mask);
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto result = index.equal_range(s);
            data.resize(s);

            auto r = reentrancy.Enter();

            for(auto i = result.first; i!=result.second; ++i)
            {
                int n = *i;  // Debug info
                assert(n>=0 && n < data.size());
                v.OnRow(&data[*i]);
            }
        }
    }    
}

int Table::Arity() const
{
    return arity;
}

const CompoundName * Table::GetCompoundName() const
{
    return name.parts.empty() ? nullptr : &name;
}

void Table::QueryDelta(Entity * row, int columns, Visitor &v)
{
    RunRules();
    
    if(deltaStart == deltaEnd)
    {
        deltaStart = 0;
        deltaEnd = data.size();
    }
    
    if(columns==0)
    {
        // This is an optimization on the next part
        // to make it slightly faster.
        for(std::size_t s=deltaStart; s<deltaEnd; s += arity)
        {
             v.OnRow(&data[s]);
        }
        return;
    }
    
    for(std::size_t s=deltaStart; s<deltaEnd; s += arity)
    {
        bool found = true;
        for(int i=0; i<arity; ++i)
            if(columns & (1<<i) && data[s+i] != row[i])
            {
                found = false;
                break;
            }
        if(found)
            v.OnRow(&data[s]);
    }
}

void Table::FirstIteration()
{
    deltaStart = 0;
    deltaEnd = data.size();
}
