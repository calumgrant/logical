#pragma once
#include "Table.hpp"
#include "../include/HashTable.hpp"

class Depth
{
public:
    class Increase
    {
    public:
        Increase(int&p) : ptr(p) { ++ptr; }
        ~Increase() { --ptr; }
        Increase(const Increase&)=delete;
    private:
        int & ptr;
    };
    
    Increase Enter() { return Increase(depth); }
    bool IsZero() const { return depth==0; }
private:
    int depth = 0;
};

class TableImpl : public Table
{
public:
    TableImpl(persist::shared_memory &mem, Arity arity);
    
    Size Rows() const override;
    void Query(Row row, Columns columns, Receiver&v) override;
    void QueryDelta(Row row, Columns columns, Receiver&v) override;
    bool QueryExists(Row row, Columns columns) override;
    void OnRow(Row row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    Arity GetArity() const override;
private:
    typedef std::vector<Entity, persist::fast_allocator<Entity>> vector;

    class Comparer
    {
        Entity::Hash hasher;
    public:
        Comparer(const vector&base, Arity arity, Columns mask);
        
        int operator()(std::size_t element) const
        {
            int h = 0;
            for(auto i = 0; i<arity; ++i)
            {
                if(mask.IsBound(i))
                    h = h * 17 + hasher(base[element + i]);
            }
            return h;
        }
        
        bool operator()(std::size_t element1, std::size_t element2) const
        {
            for(auto i = 0; i<arity; ++i)
            {
                if(mask.IsBound(i))
                {
                    if(base[element1+i] != base[element2+i]) return false;
                }
            }
            return true; // Equal
        }
    private:
        const Arity arity;
        const Columns mask;
        const vector & base;
    };
    
    persist::shared_memory & mem;
    
    const Arity arity;
    vector data;
    typedef std::unordered_set<std::size_t, Comparer, Comparer, persist::fast_allocator<std::size_t>> index_type;
    index_type hash;
    
    // Map from mask to index.
    typedef std::unordered_multiset<std::size_t, Comparer, Comparer, persist::fast_allocator<std::size_t>> map_type;
    std::unordered_map<Columns, map_type, Columns::Hash, Columns::EqualTo,
        persist::fast_allocator<std::pair<const Columns, map_type>>> indexes;
    map_type & GetIndex(Columns mask);

    Size deltaStart =0, deltaEnd = 0;
        
    void NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
};

class TableImpl2 : public Table
{
public:
    TableImpl2(persist::shared_memory &mem, Arity arity);
    Size Rows() const override;

    void Query(Row row, Columns columns, Receiver&v) override;
    void QueryDelta(Row row, Columns columns, Receiver&v) override;
    bool QueryExists(Row row, Columns columns) override;
    void OnRow(Row row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    Arity GetArity() const override;
    void NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
private:
    typedef Logical::HashColumns<Logical::DynamicArity, Logical::DynamicBinding, persist::fast_allocator<Logical::Int>> column_index;

    struct Hash
    {
        int operator()(Logical::DynamicBinding b) const { return b.mask; }
        
        bool operator()(Logical::DynamicBinding a, Logical::DynamicBinding b) const
        { return a.mask == b.mask; }
    };
    
    Logical::HashTable<Logical::DynamicArity, persist::fast_allocator<Logical::Int>> hashtable;
    
    std::unordered_map<Logical::DynamicBinding, column_index, Hash, Hash, persist::fast_allocator<std::pair<const Logical::DynamicBinding, column_index>>> indexes;
    
    column_index & GetIndex(Logical::DynamicBinding);
};
