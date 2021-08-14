
#include "Relation.hpp"

enum class ComparatorType;

/*
 Evaluates a rule/predicate.
 
 This is done by calling `Evaluate(...)` with a row of data containing
 the local evaluation state - a row containing all of the
 local variables and constants.
 
 Each Evaluation object reads/writes data to the row, then calls `next->Evaluate()`
 to evaluate the next clause in the rule. These form a chain whereby the final
 evaluation in the chain writes data into the predicate's table itself.
 
 An evaluation may call `next->Evaluate` 0, 1 or many times.
 If an evaluation fails/is none(), then `next->Evaluate()` is not called.
 */
class Evaluation : public Receiver
{
public:
    Evaluation();
    virtual ~Evaluation();
    
    void Evaluate(Entity * row)
    {
        ++callCount;
        if( ++globalCallCount <= globalCallCountLimit )
            OnRow(row);
    }
    
    virtual void Explain(Database &db, std::ostream &os, int indent=4) const =0;

    static void Indent(std::ostream &os, int indent=0);
    static void OutputVariable(std::ostream & os, int variable);
    static void OutputIntroducedVariable(std::ostream & os, int variable);
    static void OutputRelation(std::ostream &os, Database &db, const Relation & relation);

    void OutputCallCount(std::ostream&) const;
    
    static std::size_t GlobalCallCount();
    static void SetGlobalCallCountLimit(std::size_t limit);
    static std::size_t GetGlobalCallCountLimit();
    
    static void VisitSteps(EvaluationPtr & ptr, const std::function<void(EvaluationPtr&)> &fn);
    void VisitSteps(const std::function<void(Evaluation&)> & fn);

    // Options for analysis
    
    // virtual bool AlwaysEmpty() const;
    
    // Expose the variables here so that they can be manipulated by the optimizer?
    VariableId * boundVariables;
    int numberOfBoundVariables;
    VariableId * unboundVariables;
    int numberOfUnboundVariables;
    
    // Analysis results
    
    // Holds true if
    bool onRecursivePath = false;  // Output flag: r
    bool readIsRecursive = false;
    bool readDelta = false;
    
    // Optimization options:
    bool useDelta = false;
    
    bool analysed = false;
    bool runBindingAnalysis = false;
    
    // Holds true if a preceding (dominating) evaluation step is a recursive read/join.
    bool dependsOnRecursiveRead = false;  // Output flag: R
    
    SourceLocation location;

    virtual void VisitNext(const std::function<void(EvaluationPtr&, bool)> &fn);
    virtual void VisitReads(const std::function<void(Relation*&, Columns, const int*)> &fn);
    enum class VariableAccess { Read, Write, ReadWrite };
    virtual void VisitVariables(const std::function<void(int&, VariableAccess)> &fn)=0;
    virtual void VisitWrites(const std::function<void(Relation*&, int, const int*)> &fn);
        
    // Clones this node but adds a new "Next"
    virtual EvaluationPtr WithNext(const EvaluationPtr & next) const;
    virtual void BindVariable(EvaluationPtr & p, int variable);

    // Public interface to clone this evaluation
    // Also clones the internal structure (including merged branches etc).
    EvaluationPtr Clone();
    EvaluationPtr CloneInternal();
    
    virtual void EliminateWrite(EvaluationPtr & ptr, int variable);

protected:    
    // Hide the implementation
    virtual void OnRow(Row row) =0;

    EvaluationPtr clone;
    virtual EvaluationPtr MakeClone() const =0;

private:
    // The number of times Evaluate() has been called.
    std::size_t callCount;
    static std::size_t globalCallCount;
    static std::size_t globalCallCountLimit;
};

