#pragma once

#include "Entity.hpp"
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

    // Returns the number of rows.
    virtual int Count() =0;

    virtual ~Relation();
    
    virtual void AddRule(const std::shared_ptr<Evaluation> & rule) =0;
    
    virtual const std::string & Name() const =0;
    
    virtual void RunRules() =0;
};

class Predicate : public Relation
{
public:
    Predicate(Database &db, const std::string &name);
    // std::shared_ptr<Relation> data;

    // Evaluates all rules if needed
    void Evaluate();
    
    // Run rules if not already run
    void RunRules() override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    void MakeDirty();
    const std::string & Name() const override;
private:
    bool rulesRun;
    std::vector< std::shared_ptr<Evaluation> > rules;
    std::string name;
protected:
    Database &database;
};

class UnaryTable : public Predicate
{
public:
    UnaryTable(Database &db, const std::string&name);
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
    PrintRelation(std::ostream & output, Database&db, const std::string &name);
    void Add(const Entity *row) override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    int Count() override;
    void Query(Entity *row, int columns, Visitor&v) override;
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
    BinaryTable(Database &db, const std::string&name);
    void Add(const Entity * row) override;
    int Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
private:
    // This representation is inefficient - fixme.
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    std::unordered_multimap<Entity, Entity, Entity::Hash> map1, map2;
};

class TableX : public Predicate
{
public:
    TableX(Database &db, const std::string&name);
    int Count() override;
    void Query(Entity * row, int columns, Visitor&v) override;
    void Add(const Entity*row) override;
    int arity;
    std::vector<Entity> data;
};
