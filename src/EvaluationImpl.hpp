#include "Columns.hpp"

#include <vector>
#include <unordered_set>

class ChainedEvaluation : public Evaluation
{
public:
    void VisitNext(const std::function<void(EvaluationPtr&, bool)> &f) override;
    void BindVariable(EvaluationPtr & p, int variable) override;

    EvaluationPtr next;
protected:
    ChainedEvaluation(const EvaluationPtr & next);    
};

class ReaderEvaluation : public ChainedEvaluation
{
public:
    void VisitReads(const std::function<void(Relation*&rel, Columns, const int*)> & fn) override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
protected:
    ReaderEvaluation(Relation & relation, const EvaluationPtr & next);
    Relation* relation;
    Columns mask;
    std::vector<int> inputs, outputs;
    bool hasOutput;
    void UpdateHasOutput();
};

class WriterEvaluation : public Evaluation
{
protected:
    WriterEvaluation(Relation & relation);
    Relation* relation;
};

class RuleEvaluation : public ChainedEvaluation
{
public:
    RuleEvaluation(int locals, const EvaluationPtr &eval);

    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr WithNext(const EvaluationPtr & eval) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    // The number of local variables
    const int locals;
};

class OrEvaluation : public Evaluation
{
public:
    OrEvaluation(const EvaluationPtr & left, const EvaluationPtr &right);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitNext(const std::function<void(EvaluationPtr&, bool)> &f) override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
protected:
    EvaluationPtr left, right;
};

class OrEvaluationForNot : public OrEvaluation
{
public:
    OrEvaluationForNot(const EvaluationPtr &left, const EvaluationPtr &right);
    void VisitNext(const std::function<void(EvaluationPtr&, bool)> &f) override;
    EvaluationPtr MakeClone() const override;
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
    EvaluationPtr MakeClone() const override;
};

class NotTerminator : public Evaluation
{
public:
    NotTerminator(int slot);
    
    // TODO: Signal early termination if possible.
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot;
};

class BinaryEvaluation : public ChainedEvaluation
{
protected:
    BinaryEvaluation(int slot1, int slot2, const EvaluationPtr & next);
    int slot1, slot2;
};

class EqualsBB : public BinaryEvaluation
{
public:
    EqualsBB(int slot1, int slot2, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
};

class EqualsBF : public BinaryEvaluation
{
public:
    EqualsBF(int slot1, int slot2, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
    void EliminateWrite(EvaluationPtr & ptr, int variable) override;
};

class BinaryRelationEvaluation : public ReaderEvaluation
{
public:
    BinaryRelationEvaluation(Relation&, int slot1, int slot2, const EvaluationPtr & next);
    int slot1, slot2;
};

class NotInB : public ReaderEvaluation
{
public:
    NotInB(int slot, Relation & relation, const EvaluationPtr &next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot;
};

class RangeB : public ChainedEvaluation
{
public:
    RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
};

class RangeU : public ChainedEvaluation
{
public:
    RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
private:
    int slot1, slot2, slot3;
    ComparatorType cmp1, cmp2;
};

class CompareBB : public BinaryEvaluation
{
public:
    // We could have a different Evaluation type for each
    // operator for a little extra speed.
    CompareBB(int slot1, ComparatorType cmp, int slot2, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    ComparatorType cmp;
};

class NegateBF : public BinaryEvaluation
{
public:
    NegateBF(int slot1, int slot2, const EvaluationPtr & next);
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
};

class BinaryArithmeticEvaluation : public ChainedEvaluation
{
protected:
    BinaryArithmeticEvaluation(int slot1, int slot2, int slot3, const EvaluationPtr & next);
    
    int slot1, slot2, slot3;
    
    template<typename OpInt, typename OpFloat>
    void Evaluate(Entity * row);
};

class AddBBF : public BinaryArithmeticEvaluation
{
public:
    AddBBF(Database &db, int slot1, int slot2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
private:
    Database & database;  // Needed for string addition
};

class SubBBF : public BinaryArithmeticEvaluation
{
public:
    SubBBF(int slot1, int slot2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
};

class MulBBF : public BinaryArithmeticEvaluation
{
public:
    MulBBF(int slot1, int slot2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
};

class DivBBF : public BinaryArithmeticEvaluation
{
public:
    DivBBF(int slot1, int slot2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
};

class ModBBF : public BinaryArithmeticEvaluation
{
public:
    ModBBF(int slot1, int slot2, int slot3, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
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
    EvaluationPtr MakeClone() const override;
private:
    std::shared_ptr<Deduplicate> dedup;
};

class DeduplicateV : public Deduplicate
{
public:
    DeduplicateV(Database &db, const std::vector<int> & slots, const std::shared_ptr<Table> & table, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    Database & database;
    std::vector<Entity> working;
    std::vector<int> slots;
    std::shared_ptr<Table> table;
};

class DeduplicateB : public Deduplicate
{
public:
    DeduplicateB(int slot1, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot1;
    std::unordered_set<Entity, Entity::Hash> values;
};

class DeduplicateBB : public Deduplicate
{
public:
    DeduplicateBB(int slot1, int slot2, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void Reset() override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
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
    EvaluationPtr MakeClone() const override;
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
    EvaluationPtr MakeClone() const override;

private:
    int slot, sumSlot; // The slot where the value is to sum.
};

class LoadF : public ChainedEvaluation
{
public:
    LoadF(int slot, const Entity &value, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
private:
    int slot;
    Entity value;
};

class LoadB : public ChainedEvaluation
{
public:
    LoadB(int slot, const Entity &value, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot;
    Entity value;
};


class NotNone : public ChainedEvaluation
{
public:
    NotNone(int slot, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    int slot;
};

// DELETEME Dead code (superseded by Join)
class Reader : public ReaderEvaluation
{
public:
    Reader(Relation & relation, const std::vector<int> & slots, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
};

class Writer : public WriterEvaluation
{
public:
    Writer(Relation & relation, const std::vector<int> & slots, const SourceLocation & location);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    void VisitWrites(const std::function<void(Relation*&, int, const int*)> &fn) override;
    EvaluationPtr MakeClone() const override;
private:
    std::vector<int> slots;
    
    int slot;
    bool contiguous;  // Optimization
    const SourceLocation location;
};

class Join : public ReaderEvaluation
{
public:
    // -1 in an input or output means "unused"
    Join(Relation & relation, const std::vector<int> & inputs, const std::vector<int> & outputs, const EvaluationPtr & next, const SourceLocation & loc);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    EvaluationPtr MakeClone() const override;
    void BindVariable(EvaluationPtr & p, int variable) override;
    void EliminateWrite(EvaluationPtr & ptr, int variable) override;
};

class CreateNew : public ChainedEvaluation
{
public:
    CreateNew(Database &db, int slot, const EvaluationPtr & next);
    void OnRow(Entity * row) override;
    void Explain(Database &db, std::ostream &os, int indent) const override;
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override;
    EvaluationPtr MakeClone() const override;

private:
    Database & database;
    int slot;
};
