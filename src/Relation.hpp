#pragma once

#include "Entity.hpp"
#include "CompoundName.hpp"

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
    
    virtual const CompoundName * GetCompoundName() const; // Horrid name/interface
    
    virtual void RunRules() =0;
    virtual int Arity() const =0;
    
    std::size_t GetCount();
    
    virtual void AddAttribute(const std::shared_ptr<Relation> & attribute) =0;

    
    virtual void VisitAttributes(const std::function<void(Relation&)> &) const =0;
    
protected:
    // Returns the number of rows.
    virtual std::size_t Count() =0;
};
