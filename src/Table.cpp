#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"

#include "TableImpl.hpp"

#include <iostream>

TableImpl::TableImpl(persist::shared_memory & mem, Arity arity) :
    arity(arity),
    mem(mem),
    data(mem),
    hash({}, 100, Comparer(data, arity, -1), Comparer(data, arity, -1), mem),
    indexes({}, Columns::Hash(), Columns::EqualTo(), mem)
{
}

void TableImpl::OnRow(Entity *row)
{
    Add(row);
}

bool TableImpl::Add(const Entity *row)
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
        assert(row[i].Type() != EntityType::None);
    
    auto s = data.size();
    data.insert(data.end(), row, row+arity);
    
    auto i = hash.insert(s);
            
    if(!i.second)
    {
        // It was duplicated, so remove it.
        data.resize(s);
        return false;
    }
    else if(loop)
    {
        ++loop->numberOfResults;
    }
    return true;
}

void TableImpl::Clear()
{
    hash.clear();
    indexes.clear();
    data.clear();
    deltaStart = deltaEnd = 0;
}

bool TableImpl::NextIteration()
{
    bool moreResults = deltaEnd < data.size();

    //std::cout << "Next iteration: deltaStart = " << deltaStart << ", deltaEnd = " << deltaEnd << ", size = " << data.size() << std::endl;

    for(auto & index : indexes)
    {
        assert(index.second.size() == deltaEnd/arity);
    }
    
    deltaStart = deltaEnd;
    deltaEnd = data.size();

    for(std::size_t s = deltaStart; s < deltaEnd; s+= arity)
    {
        for(auto & index : indexes)
        {
            index.second.insert(s);
        }
    }

    for(auto & index : indexes)
    {
        assert(index.second.size() == deltaEnd/arity);
    }

    return moreResults;
}

TableImpl::map_type & TableImpl::GetIndex(Columns mask)
{
    auto i = indexes.find(mask);
    if(i != indexes.end())
    {
        assert(i->second.size() == deltaEnd/arity);
        return i->second;
    }
    
    // Create an index
    
    map_type it({}, 100, Comparer(data, arity, mask), Comparer(data, arity, mask), mem);
    
    auto &index = indexes.insert(std::make_pair(mask, it)).first->second;
    
    for(Size i=0; i<deltaEnd; i+=arity)
    {
        index.insert(i);
    }
    assert(index.size() == deltaEnd/arity);

    return index;
}

void TableImpl::Query(Row row, Columns mask, Receiver&v)
{
    if(mask.IsUnbound())
    {
        for(std::size_t s=0; s<deltaEnd; s+=arity)
            v.OnRow(&data[s]);
    }
    else
    {
        if(mask.IsFullyBound(arity))
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
            assert(index.size() == deltaEnd/arity);
            
            auto s = data.size();
            data.insert(data.end(), row, row+arity);
            auto result = index.equal_range(s);
            data.resize(s);

            for(auto i = result.first; i!=result.second; ++i)
            {
                int n = *i;  // Debug info
                assert(n>=0 && n < data.size());
                // Note that this only returns results up to deltaEnd because the other results aren't indexed yet.
                v.OnRow(&data[*i]);
            }
        }
    }    
}

bool TableImpl::QueryExists(Row row, Columns mask)
{
    if(mask.IsUnbound())
        return deltaEnd>0;
    else if(mask.IsFullyBound(arity))
    {
        auto s = data.size();
        data.insert(data.end(), row, row+arity);
        auto i = hash.find(s);
        data.resize(s);
        
        return i != hash.end() && *i<deltaEnd;
    }
    else
    {
        auto & index = GetIndex(mask);
        assert(index.size() == deltaEnd/arity);
        
        auto s = data.size();
        data.insert(data.end(), row, row+arity);
        auto result = index.equal_range(s);
        data.resize(s);
        
        return result.first != result.second;
    }
}

Arity TableImpl::GetArity() const
{
    return arity;
}

void TableImpl::QueryDelta(Row row, Columns columns, Receiver &v)
{
    if(columns.IsUnbound())
    {
        // This is an optimization on the next part
        // to make it slightly faster.
        for(std::size_t s=deltaStart; s<deltaEnd; s += arity)
        {
             v.OnRow(&data[s]);
        }
        return;
    }
    
    // TODO: Optimize when columns = -1 or all columns
    
    // TODO: Perhaps we should instead query the index
    for(Size s=deltaStart; s<deltaEnd; s += arity)
    {
        bool found = true;
        for(Arity i=0; i<arity; ++i)
            if(columns.IsBound(i) && data[s+i] != row[i])
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
    assert(deltaEnd == 0);
    NextIteration();
}


Size TableImpl::Rows() const
{
    return hash.size();
}

TableImpl::Comparer::Comparer(const vector & base, int arity, Columns mask) : base(base), arity(arity), mask(mask)
{
}

void TableImpl::ReadAllData(Receiver &r)    
{
    for(Size i=0; i<data.size(); i+=arity)
        r.OnRow(data.data()+i);
}
