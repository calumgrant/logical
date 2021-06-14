#pragma once

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
    
    bool evaluated = false;
    
    Database & database;
    ExecutionUnit(Database & db);
};

class Predicate : public Relation
{
public:
    Predicate(Database &db, const CompoundName &name, ::Arity arity, BindingType bindingPredicate);
    Predicate(Database &db, RelationId name, ::Arity arity, bool reaches, BindingType bindingPredicate);

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
    void VisitRules(const std::function<void(Evaluation&)> &) override;
    void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) override;
    
    void Query(Entity * row, ColumnMask columns, Receiver &r) override;
    void QueryDelta(Entity * row, ColumnMask columns, Receiver &r) override;
    void Add(const Entity * data) override;
    ::Arity Arity() const override;
    Size Count() override;
    const CompoundName * GetCompoundName() const override;
    bool IsReaches() const override;
    BindingType GetBinding() const override;
    Database & GetDatabase() const override;
    std::shared_ptr<Relation> GetBindingRelation(int columns) override;
    std::shared_ptr<Relation> GetBoundRelation(int columns) override;
    bool IsSpecial() const override;
    void FirstIteration() override;
    void NextIteration() override;

private:
#if !NDEBUG
    std::string debugName;
#endif
    bool rulesRun = false;
    const bool reaches;
    const BindingType bindingPredicate;

    RuleSet rules;
    RelationId name;
    
    std::unordered_set<std::shared_ptr<Relation>, std::hash<std::shared_ptr<Relation>>, std::equal_to<std::shared_ptr<Relation>>, persist::allocator<std::shared_ptr<Relation>>> attributes;
protected:
    Database &database;
    
    void Reset();
    
    std::shared_ptr<Table> table;
    CompoundName compoundName;
    std::unordered_map<int, std::shared_ptr<Relation>> bindingRelations, boundRelations;
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
    bool IsSpecial() const override;
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

class BuiltinFnPredicate : public Predicate
{
protected:
    BuiltinFnPredicate(Database &db, const CompoundName &name);
    void QueryDelta(Entity*row, int columns, Receiver&v) override;
    int Arity() const override;
    void AddRule(const std::shared_ptr<Evaluation> &) override;
    std::size_t Count() override;
    bool IsSpecial() const override;
    const int arity;
};

class Strlen : public BuiltinFnPredicate
{
public: 
    Strlen(Database &db);
    void Query(Entity *row, int columns, Receiver&v) override;
};

class Lowercase : public BuiltinFnPredicate
{
public:
    Lowercase(Database &db);
    void Query(Entity *row, int columns, Receiver&v) override;
};

class Uppercase : public BuiltinFnPredicate
{
public:
    Uppercase(Database &db);
    void Query(Entity *row, int columns, Receiver&v) override;
};
