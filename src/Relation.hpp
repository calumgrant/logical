#pragma once

#include "Fwd.hpp"
#include "Entity.hpp"
#include "CompoundName.hpp"
#include "Columns.hpp"
#include "SourceLocation.hpp"

#include <functional>

// A receiver "receives" rows (tuples) and is used in an evaluation pipeline
// or to receive the results of a query.
// The meaning of the row is undefined, and it is up to the user to enforce safe
// usage of the result. Bad things could happen if the row is modified in a way that
// the application is not expecting.
class Receiver
{
public:
    SourceLocation location;
    virtual void OnRow(Row row)=0;
};

enum class BindingType
{
    Unbound,
    Binding,
    Bound
};

// TODO: Move this to another file
class PredicateName
{
public:
    PredicateName();
    PredicateName(int arity, RelationId);
    CompoundName objects;
    CompoundName attributes;
    bool reaches = false;
    Arity arity;
    Columns boundColumns;
    
    void Write(Database & db, std::ostream & os) const;
    
    struct Hash
    {
        CompoundName::Hash cnh;
        
        int operator()(const PredicateName & n) const;
    };
    
    bool operator==(const PredicateName &n2) const;
    bool operator!=(const PredicateName &n2) const;
    bool operator <=(const PredicateName & n2) const;
    bool operator <(const PredicateName & n2) const;
    
    bool IsDatalog() const;
    
    // Maps a positional argument to a relation argument
    // In case they become reordered.
    int MapArgument(int arg) const;
};

class Relation
{
public:

    // Visit selected rows, based on the data in query
    // Which columns are inputs and which are outputs is unspecified.
    // Call back v.OnRow for each result,
    virtual void Query(Row query, Columns columns, Receiver &v) =0;
    virtual void QueryDelta(Row query, Columns columns, Receiver &v) =0;
    virtual bool QueryExists(Row query, Columns columns, const SourceLocation&) =0;

    // Insert a row into this table.
    virtual bool Add(const Entity * row) =0;
    virtual bool Add(const SourceLocation & loc, const Entity * row);

    virtual ~Relation();
    
    virtual void AddRule(const EvaluationPtr & rule) =0;
            
    // virtual BindingType GetBinding() const =0;
    // virtual Columns GetBindingColumns() const =0;
    
    virtual void RunRules() =0;
    
    int Arity() const;
    
    std::size_t GetCount();
    
    void VisitRules(const std::function<void(Evaluation&)> &);
    virtual void VisitRules(const std::function<void(EvaluationPtr&)> &) =0;
    
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
    bool visiting = false;
    bool analysedSemiNaive = false;
    bool enableSemiNaive = false;
    bool allowEmpty = false;

    std::shared_ptr<ExecutionUnit> loop;
    std::shared_ptr<Logical::Call> externalCall;
    PredicateName name;
    
    Logical::Call & GetExternalCall();
    
    virtual Relation& GetSemiNaivePredicate(Columns columns) =0;
    
    virtual bool IsSpecial() const =0;
    
    virtual void FirstIteration() =0;
    virtual void NextIteration() =0;
    virtual void AddExtern(Columns cols, Logical::Extern ex, void * data) =0;
    virtual void AddExtern(Logical::Extern ex, void * data) =0;
    virtual Relation & GetSemiNaive(Columns c);
    
    void LogRow(std::ostream & os, const Entity * row) const;
    virtual void Finalize() =0;
    virtual void OnStartRunningRules();
    virtual void OnStopRunningRules();

protected:
    // Returns the number of rows.
    virtual std::size_t Count() =0;
};

std::ostream & operator<<(std::ostream & os, const Relation &rel);
