#pragma once

#include <unordered_set>
#include <vector>
#include <memory>
#include <string>

class Predicate : public Relation
{
public:
    Predicate(Database &db, const CompoundName &name, ::Arity arity);
    Predicate(Database &db, RelationId name, ::Arity arity, bool reaches);

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
    void VisitRules(const std::function<void(const std::shared_ptr<Evaluation>&)> &) const override;
    void SetRecursiveRules(const std::shared_ptr<Evaluation> & baseCase, const std::shared_ptr<Evaluation> & recursiveCase) override;

    
    void Query(Entity * row, ColumnMask columns, Receiver &r) override;
    void QueryDelta(Entity * row, ColumnMask columns, Receiver &r) override;
    void Add(const Entity * data) override;
    ::Arity Arity() const override;
    Size Count() override;
    const CompoundName * GetCompoundName() const override;
    bool IsReaches() const override;
    Database & GetDatabase() const override;

private:
    bool rulesRun = false;
    const bool reaches;
    bool runBaseCase = false;
    
    std::vector< std::shared_ptr<Evaluation>, persist::allocator<std::shared_ptr<Evaluation>> > rules;
    RelationId name;
    
    // The number of
    bool evaluating;
    std::unordered_set<std::shared_ptr<Relation>, std::hash<std::shared_ptr<Relation>>, std::equal_to<std::shared_ptr<Relation>>, persist::allocator<std::shared_ptr<Relation>>> attributes;
    Size loopResults = 0;
protected:
    Database &database;
    
    std::shared_ptr<Table> table;
    CompoundName compoundName;
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
};

class PrintRelation : public SpecialPredicate
{
public:
    PrintRelation(Database&db, int name);
    void Add(const Entity *row) override;
protected:
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

