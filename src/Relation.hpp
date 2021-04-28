#include "Entity.hpp"
#include <unordered_set>
#include <vector>

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
    virtual void Query(Entity *query, Visitor &v) =0;

    // Insert a row into this table.
    virtual void Add(const Entity * row) =0;

    // Returns the number of rows.
    virtual int Count() =0;

    virtual ~Relation();
};

class Predicate : public Relation
{
public:
    Predicate();
    bool rulesRun;
    std::shared_ptr<Relation> data;
    std::vector< std::shared_ptr<Evaluation> > rules;

    // Evaluates all rules if needed
    void Evaluate();
};

class UnaryTable : public Predicate
{
public:
    void Add(const Entity *row) override;
    std::unordered_set<Entity, Entity::Hash> values;
    int Count() override;
    void Query(Entity*row, Visitor&v) override;
};

class PrintRelation : public Predicate
{
public:
    PrintRelation(Database&db);
    void Add(const Entity *row) override;
    int Count() override;
    void Query(Entity *row, Visitor&v) override;
private:
    Database &database;
};

struct PairHash
{
    int operator()(const std::pair<Entity, Entity> &value) const
    {
        Entity::Hash h;
        return h(value.first) * 13 + h(value.second);
    }
};

template<int N>
struct Row
{
    Entity data[N];
};

class BinaryTable : public Relation  // Bug: When "Predicate", there's a crash
{
public:
    void Add(const Entity * row) override;
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    int Count() override;
    void Query(Entity * row, Visitor&v) override;
};

class TableX : public Relation
{
public:
    int Count() override;
    void Query(Entity * row, Visitor&v) override;
    void Add(const Entity*row) override;
    int arity;
    std::vector<Entity> data;
};
