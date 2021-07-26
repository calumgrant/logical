#pragma once
#include "Table.hpp"
#include "../include/HashTable.hpp"

class TableImpl : public Table
{
public:
    TableImpl(AllocatorData &mem, Arity arity);
    
    Size Rows() const override;
    void Query(Row row, Columns columns, Receiver&v) override;
    void QueryDelta(Row row, Columns columns, Receiver&v) override;
    bool QueryExists(Row row, Columns columns) override;
    void OnRow(Row row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    Arity GetArity() const override;
private:
    typedef std::vector<Entity, PERSIST_ALLOCATOR<Entity>> vector;

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
    
    AllocatorData & mem;
    
    const Arity arity;
    vector data;
    typedef std::unordered_set<std::size_t, Comparer, Comparer, PERSIST_ALLOCATOR<std::size_t>> index_type;
    index_type hash;
    
    // Map from mask to index.
    typedef std::unordered_multiset<std::size_t, Comparer, Comparer, PERSIST_ALLOCATOR<std::size_t>> map_type;
    std::unordered_map<Columns, map_type, Columns::Hash, Columns::EqualTo,
        PERSIST_ALLOCATOR<std::pair<const Columns, map_type>>> indexes;
    map_type & GetIndex(Columns mask);

    Size deltaStart =0, deltaEnd = 0;
        
    void NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
};

template<typename Arity, typename Alloc=PERSIST_ALLOCATOR<Logical::Int>>
class TableImpl2 : public Table
{
public:
    TableImpl2(Alloc mem, Arity arity);
    
    template<typename Alloc2>
    TableImpl2(Alloc mem, const TableImpl2<Arity,Alloc2> &src);
    
    Size Rows() const override;

    void Query(Row row, Columns columns, Receiver&v) override;
    void QueryDelta(Row row, Columns columns, Receiver&v) override;
    bool QueryExists(Row row, Columns columns) override;
    void OnRow(Row row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    ::Arity GetArity() const override;
    void NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
    void Finalize(Database & db, std::shared_ptr<Table> & table) override;

    typedef Logical::HashColumns<Arity, Logical::DynamicBinding, Alloc> column_index;

    typedef Logical::HashColumns<Logical::StaticArity<2>, Logical::StaticBinding<true, false>, Alloc> column_index_bf;

    typedef Logical::HashColumns<Logical::StaticArity<2>, Logical::StaticBinding<false, true>, Alloc> column_index_fb;
    
    struct Hash
    {
        int operator()(Logical::DynamicBinding b) const { return b.mask; }
        
        bool operator()(Logical::DynamicBinding a, Logical::DynamicBinding b) const
        { return a.mask == b.mask; }
    };
    
    Logical::HashTable<Arity, Alloc> hashtable;
    
    using pair_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<std::pair<const Logical::DynamicBinding, column_index>>;
    
    std::unordered_map<Logical::DynamicBinding, column_index, Hash, Hash, pair_alloc> indexes;
    
    column_index & GetIndex(Logical::DynamicBinding);
};

class NonaryTable : public Table
{
public:
    NonaryTable();
    Size Rows() const override;
    void Query(Row row, Columns columns, Receiver&v) override;
    void QueryDelta(Row row, Columns columns, Receiver&v) override;
    bool QueryExists(Row row, Columns columns) override;
    void OnRow(Row row) override;
    bool Add(const Entity *e) override;
    void Clear() override;
    ::Arity GetArity() const override;
    void NextIteration() override;
    void FirstIteration() override;
    void ReadAllData(Receiver&r) override;
private:
    bool contents=false;
    bool delta=false;
};
