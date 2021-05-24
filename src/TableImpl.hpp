#pragma once

class UnaryTable : public Predicate
{
public:
    UnaryTable(Database &db, int nameId);
    void Add(const Entity *row) override;
    std::size_t Count() override;
    void Query(Entity*row, int columns, Receiver&v) override;
    void QueryDelta(Entity*row, int columns, Receiver&v) override;
    int Arity() const override;
    bool NextIteration() override;
    void FirstIteration() override;
private:
    std::unordered_set<Entity, Entity::Hash> values;
    std::shared_ptr<Relation> index;
};

class BinaryTable : public Predicate
{
public:
    BinaryTable(Database &db, int name);
    void Add(const Entity * row) override;
    std::size_t Count() override;
    void Query(Entity * row, int columns, Receiver&v) override;
    void QueryDelta(Entity*row, int columns, Receiver&v) override;
    int Arity() const override;
    bool NextIteration() override;
    void FirstIteration() override;
private:
    // This representation is inefficient - fixme.
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    std::unordered_multimap<Entity, Entity, Entity::Hash> map1, map2;
};


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

class TablePredicate : public Predicate
{
public:
    TablePredicate(Database &db, const CompoundName &name, int arity);
    TablePredicate(Database &db, RelationId name, int arity);
    std::size_t Count() override;
    void Query(Entity * row, int columns, Receiver&v) override;
    void QueryDelta(Entity*row, int columns, Receiver&v) override;
    void Add(const Entity*row) override;
    int Arity() const override;
    const CompoundName * GetCompoundName() const override;
private:

    class Comparer
    {
        Entity::Hash hasher;
    public:
        Comparer(const std::vector<Entity>&base, int arity, int mask);
        
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
        const int arity, mask;
        const std::vector<Entity> & base;
    };
    
    const int arity;
    std::vector<Entity> data;
    typedef std::unordered_set<std::size_t, Comparer, Comparer> index_type;
    index_type hash;
    
    // Map from mask to index.
    typedef std::unordered_multiset<std::size_t, Comparer, Comparer> map_type;
    std::unordered_map<int, map_type> indexes;
    map_type & GetIndex(int mask);
    
    // A count of the number times we have called Query in a nested way.
    // If reentrantDepth is 0, we can safely add rules directly to the table.
    Depth reentrancy;
    std::size_t deltaStart =0, deltaEnd = 0;
    
    const CompoundName name;
    
    bool NextIteration() override;
    void FirstIteration() override;
};
