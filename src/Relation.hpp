#pragma once

#include "Entity.hpp"
#include "CompoundName.hpp"

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

class Database;
class Entity;
class Evaluation;

class Relation
{
public:
    class Visitor
    {
    public:
        virtual void OnRow(const Entity*)=0;
    };

    // Visit selected rows, based on the data in query
    // Which columns are inputs and which are outputs is unspecified.
    // Call back v.OnRow for each result,
    virtual void Query(Entity *query, int columns, Visitor &v) =0;

    // Insert a row into this table.
    virtual void Add(const Entity * row) =0;


    virtual ~Relation();
    
    virtual void AddRule(const std::shared_ptr<Evaluation> & rule) =0;
    
    virtual int Name() const =0;
    
    virtual void RunRules() =0;
    virtual int Arity() const =0;
    
    std::size_t GetCount();
protected:
    // Returns the number of rows.
    virtual std::size_t Count() =0;
};

class Predicate : public Relation
{
public:
    Predicate(Database &db, int name);
    // std::shared_ptr<Relation> data;

    // Evaluates all rules if needed
    void Evaluate();
    
    // Run rules if not already run
    void RunRules() override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    void MakeDirty();
    int Name() const override;
    bool HasRules() const;
private:
    bool rulesRun;
    std::vector< std::shared_ptr<Evaluation> > rules;
    int name;
    
    // The number of
    bool evaluating;
    bool recursive;
    std::size_t sizeAtLastRecursiveCall;
protected:
    Database &database;
};

class UnaryTable : public Predicate
{
public:
    UnaryTable(Database &db, int nameId);
    void Add(const Entity *row) override;
    std::size_t Count() override;
    void Query(Entity*row, int columns, Visitor&v) override;
    int Arity() const override;
private:
    std::unordered_set<Entity, Entity::Hash> values;
    std::shared_ptr<Relation> index;
};

class PrintRelation : public Predicate
{
public:
    PrintRelation(std::ostream & output, Database&db, int name);
    void Add(const Entity *row) override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    std::size_t Count() override;
    void Query(Entity *row, int columns, Visitor&v) override;
    int Arity() const override;
protected:
    std::ostream & output;
};

class ErrorRelation : public PrintRelation
{
public:
    ErrorRelation(Database&db);
    void Add(const Entity *row) override;
};

class BinaryTable : public Predicate
{
public:
    BinaryTable(Database &db, int name);
    void Add(const Entity * row) override;
    std::size_t Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
    int Arity() const override;
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


class Table : public Predicate
{
public:
    Table(Database &db, int name, int arity);
    std::size_t Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
    void Add(const Entity*row) override;
    int Arity() const override;

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
    std::vector<Entity> delta_data;
    index_type delta_hash;
};

