#pragma once

#include "Entity.hpp"
#include <unordered_set>
#include <vector>
#include <memory>

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

    // Returns the number of rows.
    virtual int Count() =0;

    virtual ~Relation();
    
    virtual void AddRule(const std::shared_ptr<Evaluation> & rule) =0;
};

class Predicate : public Relation
{
public:
    Predicate();
    std::shared_ptr<Relation> data;

    // Evaluates all rules if needed
    void Evaluate();
    
    // Run rules if not already run
    void RunRules();
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    void MakeDirty();
private:
    bool rulesRun;
    std::vector< std::shared_ptr<Evaluation> > rules;
};

class UnaryTable : public Predicate
{
public:
    void Add(const Entity *row) override;
    int Count() override;
    void Query(Entity*row, int columns, Visitor&v) override;
private:
    std::unordered_set<Entity, Entity::Hash> values;
    std::shared_ptr<Relation> index;
};


class PrintRelation : public Predicate
{
public:
    PrintRelation(Database&db);
    void Add(const Entity *row) override;
    int Count() override;
    void Query(Entity *row, int columns, Visitor&v) override;
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

class BinaryTable : public Predicate
{
public:
    void Add(const Entity * row) override;
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    int Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
};

class TableX : public Predicate
{
public:
    int Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
    void Add(const Entity*row) override;
    int arity;
    std::vector<Entity> data;
};
