#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

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
    
    void AddAttribute(const std::shared_ptr<Relation> & attribute) override;
    void VisitAttributes(const std::function<void(Relation&)> &) const override;
    void VisitRules(const std::function<void(Evaluation&)> &) const override;
private:
    bool rulesRun = false;
    std::vector< std::shared_ptr<Evaluation> > rules;
    int name;
    
    // The number of
    bool evaluating;
    bool recursive;
    std::unordered_set<std::shared_ptr<Relation>> attributes;
    Size loopResults = 0;
protected:
    Database &database;
    // Notify that the next iteration has happened; release all data gathered by this iteration
    // Returns true if data was added
    virtual bool NextIteration() =0;
    virtual void FirstIteration() =0;
};



class SpecialPredicate : public Predicate
{
public:
    SpecialPredicate(Database &db, RelationId name);
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    std::size_t Count() override;
    void Query(Entity *row, int columns, Receiver&v) override;
    void QueryDelta(Entity*row, int columns, Receiver&v) override;
    int Arity() const override;
    bool NextIteration() override;
    void FirstIteration() override;
};

class PrintRelation : public SpecialPredicate
{
public:
    PrintRelation(std::ostream & output, Database&db, int name);
    void Add(const Entity *row) override;
protected:
    std::ostream & output;
};

class EvaluationStepLimit : public SpecialPredicate
{
public:
    EvaluationStepLimit(Database &db, RelationId name);
    void Add(const Entity *row) override;
};

class ExpectedResults : public SpecialPredicate
{
public:
    ExpectedResults(Database &db, RelationId name);
    void Add(const Entity *row) override;
};

class ErrorRelation : public PrintRelation
{
public:
    ErrorRelation(Database&db);
    void Add(const Entity *row) override;
};

