#include "Relation.hpp"

#include <iostream>

Table::Table(Database &db, int name, int arity) :
    Predicate(db, name), arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1)),
    delta_hash({}, 100, Comparer(delta_data, arity, -1), Comparer(delta_data, arity, -1))
{
}

void Table::Add(const Entity *row)
{
    for(int i=0; i<arity; ++i)
        assert(row[i].type != EntityType::None);
    
    if(!reentrancy.IsZero())
    {
        // We need to add the data to a delta instead.
        auto s = delta_data.size();
        delta_data.insert(delta_data.end(), row, row+arity);
        auto i = delta_hash.insert(s);
        if(!i.second)
        {
            // It was duplicated, so remove it.
            delta_data.resize(s);
        }
        return;
    }
    
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    auto i = hash.insert(s);
    if(!i.second)
    {
        // It was duplicated, so remove it.
        data.resize(s);
    }
    else
    {
    
        // Insert into all other indexes
        for(auto & index : indexes)
        {
            index.second.insert(s);
        }
    }
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
        auto r = reentrancy.Enter();
        for(int s=0; s<data.size(); s+=arity)
            v.OnRow(&data[s]);
    }
    else
    {
        
        if(mask == -1)
        {
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto i = hash.find(s);
            data.resize(s);
            
            if (i != hash.end())
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
    
    if( reentrancy.IsZero() && delta_data.size()>0 )
    {
        // Not sure this is exactly the right place to do this
        for(std::size_t s=0; s<delta_data.size(); s+=arity)
        {
            Add(&delta_data[s]);
        }
        delta_data.clear();
        delta_hash.clear();
    }
}

