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
    void VisitReads(const std::function<void(std::weak_ptr<Relation>&rel, int, const int*)> & fn) override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
protected:
    ReaderEvaluation(const std::shared_ptr<Relation> & relation, const EvaluationPtr & next);
    std::weak_ptr<Relation> relation;
    int mask;
    std::vector<int> inputs, outputs;
};

class WriterEvaluation : public Evaluation
{
protected:
    WriterEvaluation(const std::shared_ptr<Relation> & relation);
    std::weak_ptr<Relation> relation;
};

class RuleEvaluation : public ChainedEvaluation
{
public:
    RuleEvaluation(int locals, const std::shared_ptr<Evaluation> &eval);

    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    std::shared_ptr<Evaluation> WithNext(const std::shared_ptr<Evaluation> & eval) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
protected:
    std::shared_ptr<Evaluation> left, right;
};

class OrEvaluationForNot : public OrEvaluation
{
public:
    OrEvaluationForNot(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right);
    void VisitNext(const std::function<void(std::shared_ptr<Evaluation>&, bool)> &f) override;
    EvaluationPtr Clone() const override;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class NotTerminator : public Evaluation
{
public:
    NotTerminator(int slot);
    
    // TODO: Signal early termination if possible.
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot;
};

class BinaryEvaluation : public ChainedEvaluation
{
protected:
    BinaryEvaluation(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    int slot1, slot2;
};

class EqualsBB : public BinaryEvaluation
{
public:
    EqualsBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class EqualsBF : public BinaryEvaluation
{
public:
    EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class BinaryRelationEvaluation : public ReaderEvaluation
{
public:
    BinaryRelationEvaluation(const std::shared_ptr<Relation>&, int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    int slot1, slot2;
};

class NotInB : public ReaderEvaluation
{
public:
    NotInB(int slot, const std::shared_ptr<Relation> & relation, const std::shared_ptr<Evaluation> &next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot;
};

class RangeB : public ChainedEvaluation
{
public:
    RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    ComparatorType cmp;
};

class NegateBF : public BinaryEvaluation
{
public:
    NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr Clone() const override;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    Database & database;  // Needed for string addition
};

class SubBBF : public BinaryArithmeticEvaluation
{
public:
    SubBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class MulBBF : public BinaryArithmeticEvaluation
{
public:
    MulBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class DivBBF : public BinaryArithmeticEvaluation
{
public:
    DivBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class ModBBF : public BinaryArithmeticEvaluation
{
public:
    ModBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
};

class Deduplicate : public ChainedEvaluation
{
public:
    Deduplicate(const EvaluationPtr & next);
    virtual void Reset()=0;
    
    // Temporary state used to help clone a DeduplicationGuard.
    // This solution is a bit ugly.
    mutable std::shared_ptr<Deduplicate> cloneHelper;
};

class DeduplicationGuard : public ChainedEvaluation
{
public:
    DeduplicationGuard(const std::shared_ptr<Deduplicate> & dedup, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    std::shared_ptr<Deduplicate> dedup;
};

class DeduplicateV : public Deduplicate
{
public:
    DeduplicateV(Database &db, const std::vector<int> & slots, const std::shared_ptr<Table> & table, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    Database & database;
    std::vector<Entity> working;
    std::vector<int> slots;
    std::shared_ptr<Table> table;
};

class DeduplicateB : public Deduplicate
{
public:
    DeduplicateB(int slot1, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot1;
    std::unordered_set<Entity, Entity::Hash> values;
};

class DeduplicateBB : public Deduplicate
{
public:
    DeduplicateBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot1, slot2;
    std::unordered_set<std::pair<Entity,Entity>, PairHash> values;
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
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
public:
    int slot;
};

class SumCollector : public Evaluation
{
public:
    SumCollector(int slot, int target);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    const Entity & Sum() const;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;

private:
    int slot, sumSlot; // The slot where the value is to sum.
};

class Load : public ChainedEvaluation
{
public:
    Load(int slot, const Entity &value, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot;
    Entity value;
};

class NotNone : public ChainedEvaluation
{
public:
    NotNone(int slot, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    int slot;
};

class Reader : public ReaderEvaluation
{
public:
    Reader(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr Clone() const override;
};

class Writer : public WriterEvaluation
{
public:
    Writer(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    void VisitWrites(const std::function<void(std::weak_ptr<Relation>&, int, const int*)> &fn) override;
    EvaluationPtr Clone() const override;
private:
    std::vector<int> slots;
    
    int slot;
    bool contiguous;  // Optimization
};

class Join : public ReaderEvaluation
{
public:
    // -1 in an input or output means "unused"
    Join(const std::shared_ptr<Relation> & relation, const std::vector<int> & inputs, const std::vector<int> & outputs, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr Clone() const override;
};

class CreateNew : public ChainedEvaluation
{
public:
    CreateNew(Database &db, int slot, const std::shared_ptr<Evaluation> & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr Clone() const override;

private:
    Database & database;
    int slot;
};
