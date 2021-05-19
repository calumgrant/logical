
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
class Evaluation
{
public:
    Evaluation();
    virtual ~Evaluation();
    virtual void Evaluate(Entity * row) =0;  // Const
    virtual void Explain(Database &db, std::ostream &os, int indent=4) const =0;

    static void Indent(std::ostream &os, int indent=0);
    static void OutputVariable(std::ostream & os, int variable);
    static void OutputIntroducedVariable(std::ostream & os, int variable);
    static void OutputRelation(std::ostream &os, Database &db, const std::shared_ptr<Relation> & relation);
    static void OutputRelation(std::ostream &os, Database &db, const Relation & relation);

    void OutputCallCount(std::ostream&) const;
    
    static std::size_t GlobalCallCount();
    static void SetGlobalCallCountLimit(std::size_t limit);
    static std::size_t GetGlobalCallCountLimit();
    
    // Options for analysis
    
    // virtual bool AlwaysEmpty() const;
    
    // Expose the variables here so that they can be manipulated by the optimizer?
    VariableId * boundVariables;
    int numberOfBoundVariables;
    VariableId * unboundVariables;
    int numberOfUnboundVariables;
    
    bool onRecursivePath = false;
    
    // EvaluationPtr next;
    // std::weak_ptr<Relation> relation;
        
    virtual Evaluation * GetNext() const;
    virtual Evaluation * GetNext2() const;
    virtual Relation * ReadsRelation() const;
    virtual bool NextIsNot() const;
    
protected:
    // Returns false if the global call count has been exceeded
    bool IncrementCallCount()
    {
        ++callCount;
        return ++globalCallCount > globalCallCountLimit;
    }
private:
    // The number of times Evaluate() has been called.
    std::size_t callCount;
    static std::size_t globalCallCount;
    static std::size_t globalCallCountLimit;
};

