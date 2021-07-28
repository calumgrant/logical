#include "Relation.hpp"
#include "Database.hpp"
#include "RelationImpl.hpp"
#include "Helpers.hpp"
#include "TableImpl.hpp"
#include <persist.h>

#include <iostream>

TableImpl::TableImpl(AllocatorData & mem, Arity arity) :
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

template<typename Arity, typename Alloc>
TableImpl2<Arity, Alloc>::TableImpl2(Alloc mem, Arity arity) :
    hashtable(arity, mem), indexes({}, 10, Hash(), Hash(), mem)
{
}

template<typename Arity, typename Alloc>
template<typename Alloc2>
TableImpl2<Arity, Alloc>::TableImpl2(Alloc mem, const TableImpl2<Arity,Alloc2> &src) :
    hashtable(src.hashtable, mem), indexes({}, 10, Hash(), Hash(), mem)
{
}


template<typename Arity, typename Alloc>
Size TableImpl2<Arity, Alloc>::Rows() const
{
    return hashtable.size();
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::Query(Row row, Columns columns, Receiver&v)
{
    Logical::DynamicBinding binding((Logical::Int)columns.mask, Logical::DynamicArity(hashtable.arity.value));

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
    
    // TODO: Indexes for particular combinations
    
    auto & i = GetIndex(binding);

    Logical::Enumerator e;
    i.Find(e, binding, (Logical::Int*)row);
    while(i.Next(e, binding, (Logical::Int*)row))
        v.OnRow(row);
}

template<typename Arity, typename Alloc>
typename TableImpl2<Arity, Alloc>::column_index & TableImpl2<Arity, Alloc>::GetIndex(Logical::DynamicBinding binding)
{
    auto i = indexes.find(binding);
    if(i == indexes.end())
    {
        indexes.insert(std::make_pair(binding, hashtable.MakeIndex(binding)));
    }
    i = indexes.find(binding);
    return i->second;
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::QueryDelta(Row row, Columns columns, Receiver&v)
{
    Logical::Enumerator e;
    hashtable.FindDelta(e);
    Logical::DynamicBinding binding((Logical::Int)columns.mask, Logical::DynamicArity(hashtable.arity.value));
    
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

template<typename Arity, typename Alloc>
bool TableImpl2<Arity, Alloc>::QueryExists(Row row, Columns columns)
{
    Logical::DynamicBinding binding((Logical::Int)columns.mask, Logical::DynamicArity(hashtable.arity.value));

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

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::OnRow(Row row) { Add(row); }

template<typename Arity, typename Alloc>
bool TableImpl2<Arity, Alloc>::Add(const Entity *e)
{
    return hashtable.Add((const Logical::Int*)e);    
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::Clear()
{
    hashtable.clear();
    indexes.clear();
}

template<typename Arity, typename Alloc>
::Arity TableImpl2<Arity, Alloc>::GetArity() const
{
    return hashtable.arity.value;
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::NextIteration()
{
    hashtable.NextIteration();
    for(auto &i : indexes)
        i.second.NextIteration();
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::FirstIteration()
{
    NextIteration();
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::ReadAllData(Receiver&r)
{
    Logical::Enumerator e;
    auto i = hashtable.GetScanIndex();
    
    i.Find(e);
    while(auto row = i.NextRow(e))
        r.OnRow((Entity*)row);
}

NonaryTable::NonaryTable() : contents(), delta()
{
}

Size NonaryTable::Rows() const
{
    return contents;
}


void NonaryTable::Query(Row row, Columns columns, Receiver&v)
{
    if(contents) v.OnRow(row);
}

void NonaryTable::QueryDelta(Row row, Columns columns, Receiver&v)
{
    if(contents && !delta) v.OnRow(row);
}

bool NonaryTable::QueryExists(Row row, Columns columns)
{
    return contents;
}

void NonaryTable::OnRow(Row row)
{
    contents = true;
}

bool NonaryTable::Add(const Entity *e)
{
    bool added = !contents;
    contents = true;
    return added;
}

void NonaryTable::Clear()
{
    contents = false;
    delta = false;
}

::Arity NonaryTable::GetArity() const
{
    return 0;
}

void NonaryTable::NextIteration()
{
    delta = contents;
}

void NonaryTable::FirstIteration()
{
    delta = false;
}

void NonaryTable::ReadAllData(Receiver&r)
{
    if(contents) r.OnRow(nullptr);
}

std::shared_ptr<Table> Table::MakeTable(AllocatorData &mem, Arity arity)
{
    // Old implementation
    // return allocate_shared<TableImpl>(mem, mem, arity);
    
    switch(arity)
    {
        case 0:
            return allocate_shared<NonaryTable>(mem);
        case 1:
            return allocate_shared<TableImpl2<Logical::StaticArity<1>>>(mem, mem, Logical::StaticArity<1>());
        case 2:
            return allocate_shared<TableImpl2<Logical::StaticArity<2>>>(mem, mem, Logical::StaticArity<2>());
        case 3:
            return allocate_shared<TableImpl2<Logical::StaticArity<3>>>(mem, mem, Logical::StaticArity<3>());
        case 4:
            return allocate_shared<TableImpl2<Logical::StaticArity<4>>>(mem, mem, Logical::StaticArity<4>());
        default:
            return allocate_shared<TableImpl2<Logical::DynamicArity>>(mem, mem, Logical::DynamicArity(arity));
    }
}

Logical::Internal::ShortIndex Logical::Internal::GetIndexSize(int index)
{
    // Generated by the accompanying primegenerator.cpp program
    // The point is that the hash table size needs to be a prime number so that
    // there are zero collisions using quadratic hashing. If there were collisions in the same sequence
    // then the same result could be reported multiple times in a single query.
    static ShortIndex sizes[] = { 127, 251, 509, 1021, 2039, 4093, 8191, 16381, 32749, 65521, 131071,
        262139, 524287, 1048573, 2097143, 4194301, 8388593, 16777213, 33554393, 67108859, 134217689,
        268435399, 536870909, 1073741789, 2147483647 };
    return sizes[index];
}

void Table::Finalize(Database & db, std::shared_ptr<Table> & table)
{
}

template<typename Arity, typename Alloc>
void TableImpl2<Arity, Alloc>::Finalize(Database & db, std::shared_ptr<Table> & table)
{
    return;
    // Move the data to disk-memory.
    table =
        allocate_shared<TableImpl2<Arity, persist::allocator<Logical::Int>>>(db.Storage(), db.SharedMemory(), *this);
}

