#include <vector>
#include <unordered_set>

class ChainedEvaluation : public Evaluation
{
public:
    void VisitNext(const std::function<void(std::shared_ptr<Evaluation>&, bool)> &f) override;

protected:
    ChainedEvaluation(const EvaluationPtr & next);
    
    EvaluationPtr next;
};

class ReaderEvaluation : public ChainedEvaluation
{
public:
    void VisitReads(const std::function<void(std::weak_ptr<Relation>&rel, int)> & fn) override;
protected:
    ReaderEvaluation(const std::shared_ptr<Relation> & relation, const EvaluationPtr & next);
    std::weak_ptr<Relation> relation;
    int mask;
};

class WriterEvaluation : public Evaluation
{
protected:
    WriterEvaluation(const std::shared_ptr<Relation> & relation);
    std::weak_ptr<Relation> relation;
};

/*
 An evaluation over a unary predicate.
 */
class UnaryEvaluation : public ReaderEvaluation
{
protected:
    UnaryEvaluation(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    const int slot;
};

/*
 Evaluates clauses of the form f(X), where X is unbound.
 */
class EvaluateF : public UnaryEvaluation
{
public:
    EvaluateF(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next);
    void OnRow(Entity * row) override;
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
    void OnRow(Entity * row) override;
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
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

/*
 Writes data into a unary predicate.
 */
class WriterB : public WriterEvaluation
{
public:
    WriterB(const std::shared_ptr<Relation> &rel, int slot);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
};

class RuleEvaluation : public ChainedEvaluation
{
public:
    RuleEvaluation(int locals, const std::shared_ptr<Evaluation> &eval);

    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    std::shared_ptr<Evaluation> WithNext(const std::shared_ptr<Evaluation> & eval) const override;
private:
    // The number of local variables
    const int locals;
};

class OrEvaluation : public Evaluation
{
public:
    OrEvaluation(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitNext(const std::function<void(std::shared_ptr<Evaluation>&, bool)> &f) override;
protected:
    std::shared_ptr<Evaluation> left, right;
};

class OrEvaluationForNot : public OrEvaluation
{
public:
    OrEvaluationForNot(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right);
    void VisitNext(const std::function<void(std::shared_ptr<Evaluation>&, bool)> &f) override;
};


/*
    A placeholder evaluation that produces no results.
 */
class NoneEvaluation : public Evaluation
{
public:
    NoneEvaluation();
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class NotTerminator : public Evaluation
{
public:
    NotTerminator(int slot);
    
    // TODO: Signal early termination if possible.
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
};

class BinaryEvaluation : public ChainedEvaluation
{
protected:
    BinaryEvaluation(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    const int slot1, slot2;
};

class EqualsBB : public BinaryEvaluation
{
public:
    EqualsBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EqualsBF : public BinaryEvaluation
{
public:
    EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class BinaryRelationEvaluation : public ReaderEvaluation
{
public:
    BinaryRelationEvaluation(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    const int slot1, slot2;
};

class EvaluateFF : public BinaryRelationEvaluation
{
public:
    EvaluateFF(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateFB : public BinaryRelationEvaluation
{
public:
    EvaluateFB(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateBF : public BinaryRelationEvaluation
{
public:
    EvaluateBF(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class EvaluateBB : public BinaryRelationEvaluation
{
public:
    EvaluateBB(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class NotInB : public ReaderEvaluation
{
public:
    NotInB(int slot, const std::shared_ptr<Relation> & relation, const std::shared_ptr<Evaluation> &next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
};

class WriterBB : public WriterEvaluation
{
public:
    WriterBB(const std::shared_ptr<Relation>&, int slot1, int slot2);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    int slot1, slot2;
};

class RangeB : public ChainedEvaluation
{
public:
    RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
};

class RangeU : public ChainedEvaluation
{
public:
    RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
};

class CompareBB : public BinaryEvaluation
{
public:
    // We could have a different Evaluation type for each
    // operator for a little extra speed.
    CompareBB(int slot1, ComparatorType cmp, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    ComparatorType cmp;
};

class NegateBF : public BinaryEvaluation
{
public:
    NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class BinaryArithmeticEvaluation : public ChainedEvaluation
{
protected:
    BinaryArithmeticEvaluation(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    
    int slot1, slot2, slot3;
    
    template<typename OpInt, typename OpFloat>
    void Evaluate(Entity * row);
};

class AddBBF : public BinaryArithmeticEvaluation
{
public:
    AddBBF(Database &db, int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    Database & database;  // Needed for string addition
};

class SubBBF : public BinaryArithmeticEvaluation
{
public:
    SubBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class MulBBF : public BinaryArithmeticEvaluation
{
public:
    MulBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class DivBBF : public BinaryArithmeticEvaluation
{
public:
    DivBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class ModBBF : public BinaryArithmeticEvaluation
{
public:
    ModBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
};

class Deduplicate : public ChainedEvaluation
{
public:
    Deduplicate(const EvaluationPtr & next);
    virtual void Reset()=0;
};

class DeduplicationGuard : public ChainedEvaluation
{
public:
    DeduplicationGuard(const std::shared_ptr<Deduplicate> & dedup, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::shared_ptr<Deduplicate> dedup;
};

class DeduplicateB : public Deduplicate
{
public:
    DeduplicateB(int slot1, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
private:
    const int slot1;
    std::unordered_set<Entity, Entity::Hash> values;
};

class DeduplicateBB : public Deduplicate
{
public:
    DeduplicateBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
private:
    const int slot1, slot2;
    std::unordered_set<std::pair<Entity,Entity>, PairHash> values;
};

class DeduplicateV : public Deduplicate
{
public:
    DeduplicateV(Database &db, const std::vector<int> & slots, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
private:
    std::vector<int> slots;
    std::shared_ptr<Table> table;
};


/*
 Counts the number of times Evaluate has been called.
 It needs a deduplication guard in front of it.
 */
class CountCollector : public Evaluation
{
public:
    CountCollector(int slot);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
public:
    const int slot;
};

class SumCollector : public Evaluation
{
public:
    SumCollector(int slot, int target);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    const Entity & Sum() const;

private:
    const int slot, sumSlot; // The slot where the value is to sum.
};

class Load : public ChainedEvaluation
{
public:
    Load(int slot, const Entity &value, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
    Entity value;
};

class NotNone : public ChainedEvaluation
{
public:
    NotNone(int slot, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    const int slot;
};

class Reader : public ReaderEvaluation
{
public:
    Reader(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::vector<int> slots;
};

class Writer : public WriterEvaluation
{
public:
    Writer(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::vector<int> slots;
    
    int slot;
    bool contiguous;  // Optimization
};

class Join : public ReaderEvaluation
{
public:
    // -1 in an input or output means "unused"
    Join(const std::shared_ptr<Relation> & relation, std::vector<int> && inputs, std::vector<int> && outputs, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    std::vector<int> inputs, outputs;
};

class CreateNew : public ChainedEvaluation
{
public:
    CreateNew(Database &db, int slot, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
private:
    Database & database;
    const int slot;
};
