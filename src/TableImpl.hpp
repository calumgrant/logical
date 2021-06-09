#pragma once
#include "Table.hpp"

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
    void Query(Entity * row, ColumnMask columns, Receiver&v) override;
    void QueryDelta(Entity*row, ColumnMask columns, Receiver&v) override;
    void OnRow(Entity*row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    Arity GetArity() const override;
private:
    typedef std::vector<Entity, persist::fast_allocator<Entity>> vector;

    class Comparer
    {
        Entity::Hash hasher;
    public:
        Comparer(const vector&base, Arity arity, ColumnMask mask);
        
        int operator()(std::size_t element) const
        {
            int h = 0;
            for(auto i = 0; i<arity; ++i)
            {
                if(mask & (1<<i))
                    h = h * 17 + hasher(base[element + i]);
            }
            return h;
        }
        
        bool operator()(std::size_t element1, std::size_t element2) const
        {
            for(auto i = 0; i<arity; ++i)
            {
                if(mask & (1<<i))
                {
                    if(base[element1+i] != base[element2+i]) return false;
                }
            }
            return true; // Equal
        }
    private:
        const Arity arity;
        const ColumnMask mask;
        const vector & base;
    };
    
    persist::shared_memory & mem;
    
    const Arity arity;
    vector data;
    typedef std::unordered_set<std::size_t, Comparer, Comparer, persist::fast_allocator<std::size_t>> index_type;
    index_type hash;
    
    // Map from mask to index.
    typedef std::unordered_multiset<std::size_t, Comparer, Comparer, persist::fast_allocator<std::size_t>> map_type;
    std::unordered_map<ColumnMask, map_type, std::hash<ColumnMask>, std::equal_to<ColumnMask>,
        persist::fast_allocator<std::pair<const ColumnMask, map_type>>> indexes;
    map_type & GetIndex(ColumnMask mask);

    Size deltaStart =0, deltaEnd = 0;
        
    bool NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
};
