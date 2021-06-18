#pragma once

#include "Fwd.hpp"
#include "Entity.hpp"
#include "CompoundName.hpp"

// A receiver "receives" rows (tuples) and is used in an evaluation pipeline
// or to receive the results of a query.
// The meaning of the row is undefined, and it is up to the user to enforce safe
// usage of the result. Bad things could happen if the row is modified in a way that
// the application is not expecting.
class Receiver
{
public:
    virtual void OnRow(Row row)=0;
};

enum class BindingType
{
    Unbound,
    Binding,
    Bound
};

class Relation
{
public:

    // Visit selected rows, based on the data in query
    // Which columns are inputs and which are outputs is unspecified.
    // Call back v.OnRow for each result,
    virtual void Query(Row query, Columns columns, Receiver &v) =0;
    virtual void QueryDelta(Row query, Columns columns, Receiver &v) =0;

    // Insert a row into this table.
    virtual void Add(const Entity * row) =0;

    virtual ~Relation();
    
    virtual void AddRule(const EvaluationPtr & rule) =0;
    
    virtual int Name() const =0;
    
    virtual const CompoundName * GetCompoundName() const; // Horrid name/interface !!
    
    virtual bool IsReaches() const =0;
    virtual BindingType GetBinding() const =0;
    virtual Columns GetBindingColumns() const =0;
    
    virtual void RunRules() =0;
    virtual int Arity() const =0;
    
    std::size_t GetCount();
    
    virtual void AddAttribute(Relation & attribute) =0;

    virtual void VisitAttributes(const std::function<void(Relation&)> &) const =0;
    
    virtual void VisitRules(const std::function<void(Evaluation&)> &) =0;
    virtual void VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) =0;
    
    void VisitSteps(const std::function<void(EvaluationPtr&)> &);
    
    virtual Database & GetDatabase() const =0;
    
    bool analysed = false;
    bool fullyEvaluated = false;

    bool analysedForRecursion = false;  // Redundant: implied by recursiveRoot.

    // Tags for recursion
    int recursiveDepth = -1;  // -1 if this has not been visited, or >=0 if it has.
    bool parity = true;
    int recursiveRoot = -1;
    Relation * backEdge = nullptr;
    bool inRecursiveLoop = false;

    std::shared_ptr<ExecutionUnit> loop;
    
    virtual Relation& GetBindingRelation(Columns columns) =0;
    virtual Relation& GetBoundRelation(Columns columns) =0;
    
    virtual bool IsSpecial() const =0;
    
    virtual void FirstIteration() =0;
    virtual void NextIteration() =0;
    virtual void AddExtern(Columns cols, Logical::Extern ex) =0;

protected:
    // Returns the number of rows.
    virtual std::size_t Count() =0;
};

