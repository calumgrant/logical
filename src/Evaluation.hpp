#include <memory>
#include <vector>

#include "Relation.hpp"

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
    virtual ~Evaluation();
    virtual void Evaluate(Entity * row) =0;
};

/*
 An evaluation over a unary predicate.
 */
class UnaryEvaluation : public Evaluation
{
protected:
    UnaryEvaluation(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    std::weak_ptr<Relation> relation;
    int slot;
    std::shared_ptr<Evaluation> next;
};

/*
 Evaluates clauses of the form f(X), where X is unbound.
 */
class EvaluateF : public UnaryEvaluation
{
public:
    EvaluateF(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    void Evaluate(Entity * row) override;
};

/*
 Evaluates clauses of the form f(X), where X is bound,
 or a constant value.
 */
class EvaluateB : public UnaryEvaluation
{
public:
    EvaluateB(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    void Evaluate(Entity * row) override;
};

/*
 Writes data into a unary predicate.
 */
class WriterB : public Evaluation
{
public:
    WriterB(const std::shared_ptr<Relation> &rel, int slot);
    void Evaluate(Entity * row) override;
    std::shared_ptr<Relation> relation;
    int slot;
};

class RuleEvaluation : public Evaluation
{
public:
    RuleEvaluation(std::vector<Entity> && compilation, const std::shared_ptr<Evaluation> &eval);

    // Local data, pre-initialised with constants
    std::vector<Entity> row;

    void Evaluate(Entity * row) override;
private:
    std::shared_ptr<Evaluation> evaluation;
};

class OrEvaluation : public Evaluation
{
public:
    OrEvaluation(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right);
    std::shared_ptr<Evaluation> left, right;
    void Evaluate(Entity * row) override;
};

class NoneEvaluation : public Evaluation
{
public:
    void Evaluate(Entity * row) override;
};
