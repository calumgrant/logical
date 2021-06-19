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
    std::vector< std::shared_ptr<Evaluation>, persist::allocator<std::shared_ptr<Evaluation>> > rules;
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
    Predicate(Database &db, const CompoundName &name, ::Arity arity, BindingType bindingPredicate, Columns boundColumns);
    Predicate(Database &db, RelationId name, ::Arity arity, bool reaches, BindingType bindingPredicate, Columns boundColumns);

    // Evaluates all rules if needed
    void Evaluate();
    
    // Run rules if not already run
    void RunRules() override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    void MakeDirty();
    int Name() const override;
    bool HasRules() const;
    
    void AddAttribute(Relation & attribute) override;
    void VisitAttributes(const std::function<void(Relation&)> &) const override;
    void VisitRules(const std::function<void(Evaluation&)> &) override;
    void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) override;
    
    void Query(Entity * row, Columns columns, Receiver &r) override;
    void QueryDelta(Entity * row, Columns columns, Receiver &r) override;
    void Add(const Entity * data) override;
    ::Arity Arity() const override;
    Size Count() override;
    const CompoundName * GetCompoundName() const override;
    bool IsReaches() const override;
    BindingType GetBinding() const override;
    Columns GetBindingColumns() const override;
    Database & GetDatabase() const override;
    Relation& GetBindingRelation(Columns columns) override;
    Relation& GetBoundRelation(Columns columns) override;
    bool IsSpecial() const override;
    void FirstIteration() override;
    void NextIteration() override;
    void AddExtern(Columns cols, Logical::Extern ex, void * data) override;
    void AddExtern(Logical::Extern ex, void * data) override;

private:
    bool rulesRun = false;
    const bool reaches;
    const BindingType bindingPredicate;
    const Columns bindingColumns;

    RuleSet rules;
    
    std::unordered_set<Relation*, std::hash<Relation*>, std::equal_to<Relation*>, persist::allocator<Relation*>> attributes;
protected:
#if !NDEBUG
    std::string debugName;
#endif

    Database &database;
    
    void Reset();
    
    std::shared_ptr<Table> table;
    RelationId name;
    CompoundName compoundName;
    std::unordered_map<Columns, std::shared_ptr<Relation>, Columns::Hash, Columns::EqualTo> bindingRelations, boundRelations;
};

class SpecialPredicate : public Predicate // TODO: Extend Relation
{
public:
    SpecialPredicate(Database &db, RelationId name);
    SpecialPredicate(Database &db, const CompoundName & name);
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    std::size_t Count() override;
    void Query(Entity *row, Columns columns, Receiver&v) override;
    void QueryDelta(Entity*row, Columns columns, Receiver&v) override;
    bool IsSpecial() const override;
};

class ExternPredicate : public SpecialPredicate
{
public:
    ExternPredicate(Database&db, int name, const CompoundName &cn);
    
    void AddExtern(Columns c, Logical::Extern fn, void * ) override;
    void AddExtern(Logical::Extern ex, void * data) override;
    void Query(Entity *row, Columns columns, Receiver&v) override;
    void Add(const Entity * row) override;
private:
    struct ExternFn
    {
        Logical::Extern fn;
        void * data;
    };
    std::unordered_map<Columns, ExternFn, Columns::Hash, Columns::EqualTo> externs;
    ExternFn writer = { 0 };
};
