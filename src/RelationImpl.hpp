#pragma once
#include "Columns.hpp"

#include <unordered_set>
#include <vector>
#include <memory>
#include <string>

class RuleSet
{
public:
    RuleSet(Database &db);
    std::vector< std::shared_ptr<Evaluation>, PERSIST_ALLOCATOR<std::shared_ptr<Evaluation>> > rules;
    // std::unordered_set<EvaluationPtr> rules;
    
    void Add(const EvaluationPtr &ptr);
    void VisitRules(const std::function<void(Evaluation&)>&fn);
    void VisitRules(const std::function<void(EvaluationPtr&)>&fn);
    void VisitSteps(const std::function<void(EvaluationPtr&)>&fn);
    void SetRecursiveRules(const EvaluationPtr & baseCase, const EvaluationPtr & recursiveCase);
};

class ExecutionUnit
{
public:
    // Called by all predicates that add a result.
    void AddResult();
    
    Size numberOfResults = 0;
    
    RuleSet rules;
    std::unordered_set<Relation*> relations;
    
    void RunRules();
    
    void AddRelation(Relation & rel);
    void Explain();
    
    bool analysed = false;
    bool evaluated = false;
    bool recursive = false;
    
    Database & database;
    ExecutionUnit(Database & db);
    bool Recursive() const;
    Database & GetDatabase() const;
};

class Predicate : public Relation
{
public:
    Predicate(Database &db, const PredicateName &name);

    // Evaluates all rules if needed
    void Evaluate();
    
    // Run rules if not already run
    void RunRules() override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    void MakeDirty();
    bool HasRules() const;
    
    void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) override;
    
    void Query(Row row, Columns columns, Receiver &r) override;
    void QueryDelta(Row row, Columns columns, Receiver &r) override;
    bool QueryExists(Row row, Columns columns, const SourceLocation & loc) override;
    bool Add(const Entity * data) override;
    Size Count() override;
    Database & GetDatabase() const override;
    Relation& GetSemiNaivePredicate(Columns columns) override;
    bool IsSpecial() const override;
    void FirstIteration() override;
    void NextIteration() override;
    void AddExtern(Columns cols, Logical::Extern ex, void * data) override;
    void AddExtern(Logical::Extern ex, void * data) override;
    Relation & GetSemiNaive(Columns c) override;
    void Finalize() override;
private:
    bool rulesRun = false;
    bool finalized = false;

    RuleSet rules;
protected:
    
#if !NDEBUG
    // Useful for viewing in the debugger
    std::string debugName;
#endif

    Database &database;
    
    void Reset();
    
    std::shared_ptr<Table> table;
    bool sealed = false;  // Allow rules to be added.
    std::unordered_map<Columns, std::shared_ptr<Relation>, Columns::Hash, Columns::EqualTo> semiNaivePredicates;
};

class SemiNaiveQuery : public Relation
{
public:
    SemiNaiveQuery(Relation & predicate, Columns boundColumns);

    void Query(Row row, Columns c, Receiver &r) override;
    void QueryDelta(Row row, Columns c, Receiver &r) override;
    bool QueryExists(Row row, Columns c, const SourceLocation&) override;
    void AddRule(const EvaluationPtr & rule) override;
    void RunRules() override;
    void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) override;
    bool Add(const Entity * data) override;
    Size Count() override;
    Database & GetDatabase() const override;
    Relation& GetSemiNaivePredicate(Columns columns) override;
    bool IsSpecial() const override;
    void FirstIteration() override;
    void NextIteration() override;
    void AddExtern(Columns cols, Logical::Extern ex, void * data) override;
    void AddExtern(Logical::Extern ex, void * data) override;
    void Finalize() override;

private:
    Relation & underlyingPredicate;
    std::shared_ptr<Table> table;  // Bound values that are queried
};

class SemiNaivePredicate : public Relation
{
public:
    SemiNaivePredicate(Relation & predicate, Columns c, const std::shared_ptr<Table> & table);
    
    void Query(Row row, Columns c, Receiver &r) override;
    void QueryDelta(Row row, Columns c, Receiver &r) override;
    bool QueryExists(Row row, Columns c, const SourceLocation&) override;
    void AddRule(const EvaluationPtr & rule) override;
    void RunRules() override;
    void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) override;
    bool Add(const Entity * data) override;
    Size Count() override;
    Database & GetDatabase() const override;
    Relation& GetSemiNaivePredicate(Columns columns) override;
    bool IsSpecial() const override;
    void FirstIteration() override;
    void NextIteration() override;
    void AddExtern(Columns cols, Logical::Extern ex, void * data) override;
    void AddExtern(Logical::Extern ex, void * data) override;
    void Finalize() override;

    void CopyRules();

private:
    Relation& underlyingPredicate;
    RuleSet rules;
    std::shared_ptr<SemiNaiveQuery> query;
    std::shared_ptr<Table> table;  // Where the results are stored (shared with the underlying predicate
    
    bool AddQuery(Row query);
    void RunRules(Row query);
    EvaluationPtr MakeBoundRule(const EvaluationPtr & rule);
};

class SpecialPredicate : public Predicate // TODO: Extend Relation
{
public:
    SpecialPredicate(Database &db, const PredicateName & name);
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    std::size_t Count() override;
    void Query(Entity *row, Columns columns, Receiver&v) override;
    void QueryDelta(Entity*row, Columns columns, Receiver&v) override;
    bool QueryExists(Entity * row, Columns columns, const SourceLocation&) override;
    bool IsSpecial() const override;
};

class ExternPredicate : public SpecialPredicate
{
public:
    ExternPredicate(Database&db, const PredicateName &name);
    
    void AddExtern(Columns c, Logical::Extern fn, void * ) override;
    void AddExtern(Logical::Extern ex, void * data) override;
    void Query(Entity *row, Columns columns, Receiver&v) override;
    bool Add(const SourceLocation & location, const Entity * row) override;
    bool Add(const Entity * row) override;
    void AddVarargs(Logical::Extern ex, void * data);
    void OnStartRunningRules() override;
    void OnStopRunningRules() override;
private:
    struct ExternFn
    {
        Logical::Extern fn;
        void * data;
    };
    std::unordered_map<Columns, ExternFn, Columns::Hash, Columns::EqualTo> externs;
    ExternFn writer = { 0 }, varargs = { 0 };
};
