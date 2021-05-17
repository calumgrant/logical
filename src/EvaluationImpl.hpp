#include <vector>
#include <unordered_set>

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
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

/*
 Evaluates clauses of the form f(X), where X is unbound.
 Only calls next->Evaluate() 0 or 1 times.
 This is in circumstances where the variable is not used again
 in the clause, so it becomes an existence tests rather than
 needing to enumerate the entire table.
 */
class ExistsF : public UnaryEvaluation
{
    ExistsF(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
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
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

/*
 Writes data into a unary predicate.
 */
class WriterB : public Evaluation
{
public:
    WriterB(const std::shared_ptr<Relation> &rel, int slot);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::weak_ptr<Relation> relation;
    int slot;
};

class RuleEvaluation : public Evaluation
{
public:
    RuleEvaluation(int locals, const std::shared_ptr<Evaluation> &eval);

    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::shared_ptr<Evaluation> evaluation;
    
    // The number of local variables
    int locals;
};

class OrEvaluation : public Evaluation
{
public:
    OrEvaluation(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::shared_ptr<Evaluation> left, right;
};

/*
    A placeholder evaluation that produces no results.
 */
class NoneEvaluation : public Evaluation
{
public:
    NoneEvaluation();
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class NotTerminator : public Evaluation
{
public:
    NotTerminator(int slot);
    
    // TODO: Signal early termination if possible.
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
};

class BinaryEvaluation : public Evaluation
{
protected:
    BinaryEvaluation(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    int slot1, slot2;
    std::shared_ptr<Evaluation> next;

};

class EqualsBB : public BinaryEvaluation
{
public:
    EqualsBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EqualsBF : public BinaryEvaluation
{
public:
    EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class BinaryRelationEvaluation : public BinaryEvaluation
{
public:
    BinaryRelationEvaluation(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
protected:
    std::weak_ptr<Relation> relation;
};

class EvaluateFF : public BinaryRelationEvaluation
{
public:
    EvaluateFF(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateFB : public BinaryRelationEvaluation
{
public:
    EvaluateFB(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateBF : public BinaryRelationEvaluation
{
public:
    EvaluateBF(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateBB : public BinaryRelationEvaluation
{
public:
    EvaluateBB(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class NotInB : public Evaluation
{
public:
    NotInB(int slot, const std::shared_ptr<Relation> & relation, const std::shared_ptr<Evaluation> &next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
    const std::weak_ptr<Relation> relation;
    const std::shared_ptr<Evaluation> next;
};

class WriterBB : public Evaluation
{
public:
    WriterBB(const std::shared_ptr<Relation>&, int slot1, int slot2);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::weak_ptr<Relation> relation;
    int slot1, slot2;
};

class RangeB : public Evaluation
{
public:
    RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
    std::shared_ptr<Evaluation> next;
};

class RangeU : public Evaluation
{
public:
    RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
    std::shared_ptr<Evaluation> next;
};

class CompareBB : public BinaryEvaluation
{
public:
    // We could have a different Evaluation type for each
    // operator for a little extra speed.
    CompareBB(int slot1, ComparatorType cmp, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    ComparatorType cmp;
};

class NegateBF : public BinaryEvaluation
{
public:
    NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class BinaryArithmeticEvaluation : public Evaluation
{
protected:
    BinaryArithmeticEvaluation(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    
    int slot1, slot2, slot3;
    std::shared_ptr<Evaluation> next;
    
    template<typename OpInt, typename OpFloat>
    void Evaluate(Entity * row);
};

class AddBBF : public BinaryArithmeticEvaluation
{
public:
    AddBBF(Database &db, int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    Database & database;  // Needed for string addition
};

class SubBBF : public BinaryArithmeticEvaluation
{
public:
    SubBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class MulBBF : public BinaryArithmeticEvaluation
{
public:
    MulBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class DivBBF : public BinaryArithmeticEvaluation
{
public:
    DivBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class ModBBF : public BinaryArithmeticEvaluation
{
public:
    ModBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class DeduplicateB : public Evaluation
{
public:
    DeduplicateB(int slot1, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot1;
    std::unordered_set<Entity, Entity::Hash> values;
    const std::shared_ptr<Evaluation> next;
};

class DeduplicateBB : public Evaluation
{
public:
    DeduplicateBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot1, slot2;
    std::unordered_set<std::pair<Entity,Entity>, PairHash> values;
    const std::shared_ptr<Evaluation> next;
};


/*
 Counts the number of times Evaluate has been called.
 It needs a deduplication guard in front of it.
 */
class CountCollector : public Evaluation
{
public:
    CountCollector(int slot);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
public:
    const int slot;
};

class SumCollector : public Evaluation
{
public:
    SumCollector(int slot, int target);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    const Entity & Sum() const;

private:
    const int slot, sumSlot; // The slot where the value is to sum.
};

class Load : public Evaluation
{
public:
    Load(int slot, const Entity &value, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
    Entity value;
    const std::shared_ptr<Evaluation> next;
};

class NotNone : public Evaluation
{
public:
    NotNone(int slot, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
    const std::shared_ptr<Evaluation> next;
};

class Reader : public Evaluation
{
public:
    Reader(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::weak_ptr<Relation> relation;
    std::vector<int> slots;
    std::shared_ptr<Evaluation> next;
};

class Writer : public Evaluation
{
public:
    Writer(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::weak_ptr<Relation> relation;
    std::vector<int> slots;
    
    int slot;
    bool contiguous;  // Optimization
};

class Join : public Evaluation
{
public:
    // -1 in an input or output means "unused"
    Join(const std::shared_ptr<Relation> & relation, std::vector<int> && inputs, std::vector<int> && outputs, const std::shared_ptr<Evaluation> & next);
    void Evaluate(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::weak_ptr<Relation> relation;
    std::vector<int> inputs, outputs;
    std::shared_ptr<Evaluation> next;
    int mask;
};