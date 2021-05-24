#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"

#include "TableImpl.hpp"

#include <iostream>

TableImpl::TableImpl(Arity arity) :
    arity(arity),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1))
{
}

void TableImpl::OnRow(Entity *row)
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
    else if(loop)
    {
        ++loop->numberOfResults;
    }
}

bool TableImpl::NextIteration()
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

TableImpl::map_type & TableImpl::GetIndex(int mask)
{
    auto i = indexes.find(mask);
    if(i != indexes.end())
    {
        return i->second;
    }
    
    // Create an index
    
    map_type it({}, 100, Comparer(data, arity, mask), Comparer(data, arity, mask));
    
    auto &index = indexes.insert(std::make_pair(mask, it)).first->second;
    
    for(auto i=0; i<deltaEnd; i+=arity)
    {
        index.insert(i);
    }
    
    return index;
}

void TableImpl::Query(Entity * row, int mask, Receiver&v)
{
    if(mask==0)
    {
        for(std::size_t s=0; s<deltaEnd; s+=arity)
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
            
            if (i != hash.end() && *i<deltaEnd)
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

            for(auto i = result.first; i!=result.second; ++i)
            {
                int n = *i;  // Debug info
                assert(n>=0 && n < data.size());
                // Note that this only returns results up to deltaEnd because the other results aren't indexed.
                v.OnRow(&data[*i]);
            }
        }
    }    
}

Arity TableImpl::GetArity() const
{
    return arity;
}

void TableImpl::QueryDelta(Entity * row, int columns, Receiver &v)
{
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
    
    // TODO: Perhaps we should instead query the index
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

void TableImpl::FirstIteration()
{
    deltaStart = 0;
    deltaEnd = data.size();
}


Size TableImpl::Rows() const
{
    return hash.size();
}

TableImpl::Comparer::Comparer(const std::vector<Entity> & base, int arity, int mask) : base(base), arity(arity), mask(mask)
{
}
