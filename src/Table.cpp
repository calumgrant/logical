#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"
#include "Helpers.hpp"
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

void TableImpl::NextIteration()
{
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

TableImpl2::TableImpl2(persist::shared_memory & mem, Arity arity) :
    hashtable(Logical::DynamicArity(arity), mem), indexes({}, 10, Hash(), Hash(), mem)
{
}

Size TableImpl2::Rows() const
{
    return hashtable.size();
}

void TableImpl2::Query(Row row, Columns columns, Receiver&v)
{
    Logical::DynamicBinding binding((Logical::Int)columns.mask, hashtable.arity);

    if(columns.IsUnbound())
    {
        // This is a scan
        Logical::Enumerator e;
        hashtable.Find(e, binding);
        while(auto data = hashtable.NextRow(e))
            v.OnRow((Row)data);
        return;
    }
    
    if(columns.IsFullyBound(hashtable.get_arity()))
    {
        // This is a probe
        auto i = hashtable.GetProbeIndex();
        Logical::Enumerator e;

        i.Find(e, binding, (Logical::Int*)row);
        if(i.Next(e, binding, (Logical::Int*)row))
            v.OnRow(row);
        return;
    }
    
    auto & i = GetIndex(binding);

    Logical::Enumerator e;
    i.Find(e, binding, (Logical::Int*)row);
    while(i.Next(e, binding, (Logical::Int*)row))
        v.OnRow(row);
}

TableImpl2::column_index & TableImpl2::GetIndex(Logical::DynamicBinding binding)
{
    auto i = indexes.find(binding);
    if(i == indexes.end())
    {
        indexes.insert(std::make_pair(binding, hashtable.MakeIndex(binding)));
    }
    i = indexes.find(binding);
    return i->second;
}

void TableImpl2::QueryDelta(Row row, Columns columns, Receiver&v)
{
    Logical::Enumerator e;
    hashtable.FindDelta(e);
    Logical::DynamicBinding binding((Logical::Int)columns.mask, hashtable.arity);
    
    if(columns.IsUnbound())
    {
        while(auto result = hashtable.NextRow(e))
            v.OnRow((Row)result);
        return;

    }

    while(auto result = hashtable.NextRow(e))
    {
        if(Logical::Internal::BoundEquals(binding, result, (Logical::Int*)row))
        {
            Logical::Internal::BindRow(binding, result, (Logical::Int*)row);
            v.OnRow(row);
//            v.OnRow((Row)result);
        }
    }
}

bool TableImpl2::QueryExists(Row row, Columns columns)
{
    Logical::DynamicBinding binding((Logical::Int)columns.mask, hashtable.arity);

    if(columns.IsUnbound())
    {
        // This is a scan
        Logical::Enumerator e;
        hashtable.Find(e, binding);
        return hashtable.NextRow(e);
    }
    
    if(columns.IsFullyBound(hashtable.get_arity()))
    {
        // This is a probe
        auto i = hashtable.GetProbeIndex();
        Logical::Enumerator e;

        i.Find(e, binding, (Logical::Int*)row);
        return i.Next(e, binding, (Logical::Int*)row);
    }

    auto & i = GetIndex(binding);

    Logical::Enumerator e;
    i.Find(e, binding, (Logical::Int*)row);
    return i.Next(e, binding, (Logical::Int*)row);
}

void TableImpl2::OnRow(Row row) { Add(row); }

bool TableImpl2::Add(const Entity *e)
{
    auto added = hashtable.Add((const Logical::Int*)e);
    
    if(added && loop)
        ++loop->numberOfResults;
    
    return added;
}

void TableImpl2::Clear()
{
    hashtable.clear();
    indexes.clear();
}

Arity TableImpl2::GetArity() const
{
    return hashtable.arity.value;
}

void TableImpl2::NextIteration()
{
    hashtable.NextIteration();
    for(auto &i : indexes)
        i.second.NextIteration();
}

void TableImpl2::FirstIteration()
{
    NextIteration();
}

void TableImpl2::ReadAllData(Receiver&r)
{
    Logical::Enumerator e;
    auto i = hashtable.GetScanIndex();
    
    i.Find(e);
    while(auto row = i.NextRow(e))
        r.OnRow((Entity*)row);
}

std::shared_ptr<Table> Table::MakeTable(persist::shared_memory &mem, Arity arity)
{
    return allocate_shared<TableImpl2>(mem, mem, arity);
}
