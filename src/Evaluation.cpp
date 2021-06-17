#include "Evaluation.hpp"
#include "Database.hpp"
#include "AST.hpp"
#include "Colours.hpp"
#include "EvaluationImpl.hpp"
#include "TableImpl.hpp"

#include <iostream>
#include <sstream>

Evaluation::~Evaluation()
{
}

NoneEvaluation::NoneEvaluation()
{
    // This is useful for a breakpoint
}

void NoneEvaluation::OnRow(Entity *row)
{
}

OrEvaluation::OrEvaluation(const std::shared_ptr<Evaluation> &lhs, const std::shared_ptr<Evaluation> &rhs) : left(lhs), right(rhs)
{
}

void OrEvaluation::OnRow(Entity *row)
{
    left->Evaluate(row);
    right->Evaluate(row);
}

RuleEvaluation::RuleEvaluation(int locals, const std::shared_ptr<Evaluation> &eval) : ChainedEvaluation(eval),
                                                                                      locals(locals)
{
}

void RuleEvaluation::OnRow(Entity *)
{
    std::vector<Entity> row(locals);
    next->Evaluate(&row[0]);
}

class UnaryVisitor : public Receiver
{
public:
    UnaryVisitor(Entity *row, int slot, Evaluation &next) : row(row), slot(slot), next(next)
    {
    }

    void OnRow(Entity *e) override
    {
        row[slot] = e[0];
        next.Evaluate(row);
    }

    Entity *row;
    int slot;
    Evaluation &next;
};

void OrEvaluation::Explain(Database &db, std::ostream &os, int indent) const
{
    left->Explain(db, os, indent);
    right->Explain(db, os, indent);
}

void NoneEvaluation::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "None";
    OutputCallCount(os);
    os << "\n";
}

void Evaluation::Indent(std::ostream &os, int indent)
{
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
}

void RuleEvaluation::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Evaluate with " << locals << " variables";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

NotTerminator::NotTerminator(int slot) : slot(slot)
{
}

void NotTerminator::OnRow(Entity *row)
{
    row[slot] = Entity{EntityType::None, 0};
}

void NotTerminator::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Load ";
    OutputIntroducedVariable(os, slot);
    os << " := ";
    db.PrintQuoted(Entity(), os);
    OutputCallCount(os);
    os << "\n";
}

BinaryEvaluation::BinaryEvaluation(int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : slot1(slot1), slot2(slot2), ChainedEvaluation(next)
{
}

EqualsBB::EqualsBB(int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : BinaryEvaluation(slot1, slot2, next)
{
}

void EqualsBB::OnRow(Entity *row)
{
    if (row[slot1] == row[slot2])
        next->Evaluate(row);
}

void EqualsBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " == _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

EqualsBF::EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : BinaryEvaluation(slot1, slot2, next)
{
}

void EqualsBF::OnRow(Entity *row)
{
    row[slot2] = row[slot1];
    next->Evaluate(row);
}

void EqualsBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Assign ";
    OutputIntroducedVariable(os, slot2);
    os << " := ";
    OutputVariable(os, slot1);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

BinaryRelationEvaluation::BinaryRelationEvaluation(Relation &relation, int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : ReaderEvaluation(relation, next), slot1(slot1), slot2(slot2)
{
}

RangeB::RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> &next) : slot1(slot1), slot2(slot2), slot3(slot3), cmp1(cmp1), cmp2(cmp2), ChainedEvaluation(next)
{
}

template <typename T>
bool Compare(T value1, T value2, ComparatorType t)
{
    switch (t)
    {
    case ComparatorType::eq:
        return value1 == value2;
        break;
    case ComparatorType::neq:
        return value1 != value2;
        break;
    case ComparatorType::lt:
        return value1 < value2;
        break;
    case ComparatorType::gt:
        return value1 > value2;
        break;
    case ComparatorType::lteq:
        return value1 <= value2;
        break;
    case ComparatorType::gteq:
        return value1 >= value2;
        break;
    }
    assert(!"Invalid comparator");
    return false;
}

bool Compare(const Entity &e1, const Entity &e2, ComparatorType cmp)
{
    auto t1 = e1.Type(), t2 = e2.Type();

    if (t1 == EntityType::Integer && t2 == EntityType::Integer)
        return Compare((std::int64_t)e1, (std::int64_t)e2, cmp);
    else if (t1 == EntityType::Integer && t2 == EntityType::Float)
        return Compare<float>((std::int64_t)e1, (double)e2, cmp);
    else if (t1 == EntityType::Float && t2 == EntityType::Integer)
        return Compare<float>((double)e1, (std::int64_t)e2, cmp);
    else if (t1 == EntityType::Float && t2 == EntityType::Float)
        return Compare((double)e1, (double)e2, cmp);
    else
        return false;
}

void RangeB::OnRow(Entity *row)
{
    if (Compare(row[slot1], row[slot2], cmp1) && Compare(row[slot2], row[slot3], cmp2))
        next->Evaluate(row);
}

void RangeB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " " << cmp1 << " _" << slot2 << " and _" << slot2 << " " << cmp2 << " _" << slot3;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

RangeU::RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> &next) : slot1(slot1), slot2(slot2), slot3(slot3), cmp1(cmp1), cmp2(cmp2), ChainedEvaluation(next)
{
}

void RangeU::OnRow(Entity *row)
{
    std::int64_t lowerBound, upperBound;

    if (row[slot1].IsInt())
    {
        lowerBound = cmp1 == ComparatorType::lt ? (std::int64_t)row[slot1] + 1 : (std::int64_t)row[slot1];
    }
    else
        return; // For now, floats are not supported

    if (row[slot3].IsInt())
    {
        upperBound = cmp2 == ComparatorType::lt ? (std::int64_t)row[slot3] - 1 : (std::int64_t)row[slot3];
    }
    else
        return; // For now, floats are not supported

    for (auto value = lowerBound; value <= upperBound; ++value)
    {
        row[slot2] = Entity(EntityType::Integer, value);
        next->Evaluate(row);
    }
}

void RangeU::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "For ";
    OutputVariable(os, slot1);
    os << " " << cmp1 << " ";
    OutputIntroducedVariable(os, slot2);
    os << " " << cmp2 << " ";
    OutputVariable(os, slot3);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

CompareBB::CompareBB(int slot1, ComparatorType cmp, int slot2, const std::shared_ptr<Evaluation> &next) : BinaryEvaluation(slot1, slot2, next), cmp(cmp)
{
}

void CompareBB::OnRow(Entity *row)
{
    // Note: We don't compare strings for inequality.
    // FIXME
    if (Compare(row[slot1], row[slot2], cmp))
        next->Evaluate(row);
}

void CompareBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test ";
    OutputVariable(os, slot1);
    os << " " << cmp << " ";
    OutputVariable(os, slot2);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

NegateBF::NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : BinaryEvaluation(slot1, slot2, next)
{
}

void NegateBF::OnRow(Entity *row)
{
    switch (row[slot1].Type())
    {
    case EntityType::Integer:
        row[slot2] = Entity(EntityType::Integer, -(std::int64_t)row[slot1]);
        break;
    case EntityType::Float:
        row[slot2] = Entity(EntityType::Float, -(double)row[slot1]);
        break;
    default:
        return; // Fail
    }
    next->Evaluate(row);
}

void NegateBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot2 << " := -_" << slot1;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

BinaryArithmeticEvaluation::BinaryArithmeticEvaluation(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : slot1(slot1), slot2(slot2), slot3(slot3), ChainedEvaluation(next)
{
}

AddBBF::AddBBF(Database &db, int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : BinaryArithmeticEvaluation(slot1, slot2, slot3, next), database(db)
{
}

SubBBF::SubBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

MulBBF::MulBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

DivBBF::DivBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

ModBBF::ModBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> &next) : BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

void AddBBF::OnRow(Entity *row)
{
    auto t1 = row[slot1].Type();
    auto t2 = row[slot2].Type();

    if (t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3] = Entity(EntityType::Integer, (std::int64_t)row[slot1] + (std::int64_t)row[slot2]);
    }
    else if (t1 == EntityType::String && t2 == EntityType::String)
    {
        row[slot3] = database.AddStrings((std::int64_t)row[slot1], (std::int64_t)row[slot2]);
    }
    else if (t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3] = (double)row[slot1] + (double)row[slot2];
    }
    else if (t1 == EntityType::String || t2 == EntityType::String)
    {
        // Convert the other to a string
        if (t1 == EntityType::Integer)
        {
            std::ostringstream ss;
            ss << (std::int64_t)row[slot1] << database.GetString((std::int64_t)row[slot2]);
            row[slot3] = database.CreateString(ss.str().c_str());
        }
        else if (t1 == EntityType::Float)
        {
            std::ostringstream ss;
            ss << (double)row[slot1] << database.GetString((std::int64_t)row[slot2]);
            row[slot3] = database.CreateString(ss.str().c_str());
        }
        else if (t2 == EntityType::Integer)
        {
            std::ostringstream ss;
            ss << database.GetString((std::int64_t)row[slot1]) << (std::int64_t)row[slot2];
            row[slot3] = database.CreateString(ss.str().c_str());
        }
        else if (t2 == EntityType::Float)
        {
            std::ostringstream ss;
            ss << database.GetString((std::int64_t)row[slot1]) << (double)row[slot2];
            row[slot3] = database.CreateString(ss.str().c_str());
        }
        else
            return;
    }
    else if (t1 == EntityType::Float || t2 == EntityType::Float)
    {
        // Convert the other to a float
        if (t1 == EntityType::Integer)
        {
            row[slot3] = (std::int64_t)row[slot1] + (double)row[slot2];
        }
        else if (t2 == EntityType::Integer)
        {
            row[slot3] = (double)row[slot1] + (std::int64_t)row[slot2];
        }
        else
            return;
    }
    else
    {
        return;
    }
    next->Evaluate(row);
}

void AddBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate ";
    OutputIntroducedVariable(os, slot3);
    os << " := ";
    OutputVariable(os, slot1);
    os << " + ";
    OutputVariable(os, slot2);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

template <typename OpInt, typename OpFloat>
void BinaryArithmeticEvaluation::Evaluate(Entity *row)
{
    auto t1 = row[slot1].Type();
    auto t2 = row[slot2].Type();

    if (t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3] = Entity(EntityType::Integer, OpInt()((std::int64_t)row[slot1], (std::int64_t)row[slot2]));
    }
    else if (t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3] = OpFloat()((double)row[slot1], (double)row[slot2]);
    }
    else if (t1 == EntityType::Float && t2 == EntityType::Integer)
    {
        row[slot3] = OpFloat()((double)row[slot1], (std::int64_t)row[slot2]);
    }
    else if (t1 == EntityType::Integer && t2 == EntityType::Float)
    {
        row[slot3] = OpFloat()((std::int64_t)row[slot1], (double)row[slot2]);
    }
    else
        return;

    next->Evaluate(row);
}

void SubBBF::OnRow(Entity *row)
{
    BinaryArithmeticEvaluation::Evaluate<std::minus<int>, std::minus<float>>(row);
}

void SubBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate ";
    OutputIntroducedVariable(os, slot3);
    os << " := ";
    OutputVariable(os, slot1);
    os << " - ";
    OutputVariable(os, slot2);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

void MulBBF::OnRow(Entity *row)
{
    BinaryArithmeticEvaluation::Evaluate<std::multiplies<int>, std::multiplies<float>>(row);
}

void MulBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " * _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

void Evaluation::OutputCallCount(std::ostream &os) const
{
    os << Colours::Detail << " (called " << callCount << " time";
    if (callCount != 1)
        os << "s";

    if (onRecursivePath || dependsOnRecursiveRead)
    {
        os << ", flags:";
        if (onRecursivePath)
            os << "r";
        if (dependsOnRecursiveRead)
            os << "d";
        if (readIsRecursive)
            os << "R";
        if (readDelta)
            os << "D";
    }
    os << ")" << Colours::Normal;
}

void DivBBF::OnRow(Entity *row)
{
    auto t1 = row[slot1].Type();
    auto t2 = row[slot2].Type();

    if (t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        if ((std::int64_t)row[slot2] == 0)
            return;

        row[slot3] = Entity{EntityType::Integer, (std::int64_t)row[slot1] / (std::int64_t)row[slot2]};
    }
    else if (t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3] = (double)row[slot1] / (double)row[slot2];
    }
    else if (t1 == EntityType::Float && t2 == EntityType::Integer)
    {
        row[slot3] = (double)row[slot1] / (std::int64_t)row[slot2];
    }
    else if (t1 == EntityType::Integer && t2 == EntityType::Float)
    {
        row[slot3] = (std::int64_t)row[slot1] / (double)row[slot2];
    }
    else
        return;

    next->Evaluate(row);
}

void DivBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " / _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

void ModBBF::OnRow(Entity *row)
{
    auto t1 = row[slot1].Type();
    auto t2 = row[slot1].Type();

    if (t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        if ((std::int64_t)row[slot2] == 0)
            return;
        row[slot3] = Entity{EntityType::Integer, (std::int64_t)row[slot1] % (std::int64_t)row[slot2]};
    }
    else
        return;
    next->Evaluate(row);
}

void ModBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " % _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

Evaluation::Evaluation() : callCount(0)
{
}

DeduplicateB::DeduplicateB(int slot1, const std::shared_ptr<Evaluation> &next) : slot1(slot1), Deduplicate(next)
{
}

void DeduplicateB::OnRow(Entity *row)
{
    auto i = values.insert(row[slot1]);
    if (i.second)
    {
        next->Evaluate(row);
    }
}

void DeduplicateB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Deduplicate ";
    OutputVariable(os, slot1);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

DeduplicateBB::DeduplicateBB(int slot1, int slot2, const std::shared_ptr<Evaluation> &next) : slot1(slot1), slot2(slot2), Deduplicate(next)
{
}

void DeduplicateBB::OnRow(Entity *row)
{
    auto i = values.insert(std::make_pair(row[slot1], row[slot2]));
    if (i.second)
    {
        next->Evaluate(row);
    }
}

void DeduplicateBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Deduplicate (_" << slot1 << ",_" << slot2 << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

CountCollector::CountCollector(int slot) : slot(slot)
{
}

void CountCollector::OnRow(Entity *row)
{
    row[slot] = Entity{EntityType::Integer, 1 + (std::int64_t)row[slot]};
}

void CountCollector::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Increment ";
    OutputVariable(os, slot);
    OutputCallCount(os);
    os << std::endl;
}

SumCollector::SumCollector(int slot, int sumSlot) : slot(slot), sumSlot(sumSlot)
{
}

void SumCollector::OnRow(Entity *row)
{
    row[sumSlot] += row[slot];
}

void SumCollector::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Sum _" << slot << " into _" << sumSlot;
    OutputCallCount(os);
    std::cout << std::endl;
}

Load::Load(int slot, const Entity &v, const std::shared_ptr<Evaluation> &next) : slot(slot), value(v), ChainedEvaluation(next)
{
}

void Load::OnRow(Entity *locals)
{
    locals[slot] = value;
    next->Evaluate(locals);
}

void Load::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Load ";
    OutputIntroducedVariable(os, slot);
    os << " := ";
    db.PrintQuoted(value, os);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

NotNone::NotNone(int slot, const std::shared_ptr<Evaluation> &next) : slot(slot), ChainedEvaluation(next)
{
}

void NotNone::OnRow(Entity *row)
{
    if (!row[slot].IsNone())
        next->Evaluate(row);
}

void NotNone::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Check ";
    OutputVariable(os, slot);
    os << " is not ";
    db.PrintQuoted(Entity(), os);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

NotInB::NotInB(int slot, Relation &relation, const std::shared_ptr<Evaluation> &next) : slot(slot), ReaderEvaluation(relation, next)
{
}

void NotInB::OnRow(Entity *row)
{
    class Visitor : public Receiver
    {
    public:
        bool found = false;
        void OnRow(Entity *) override { found = true; }
    } visitor;
    relation->Query(row + slot, 1, visitor);

    if (!visitor.found)
        next->Evaluate(row);
}

void NotInB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Lookup _" << slot << " is not in " << db.GetString(relation->Name());
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

std::size_t Evaluation::globalCallCount = 0;
std::size_t Evaluation::globalCallCountLimit = -1;

std::size_t Evaluation::GlobalCallCount()
{
    return globalCallCount;
}

Reader::Reader(Relation &relation, const std::vector<int> &slots, const std::shared_ptr<Evaluation> &next) : ReaderEvaluation(relation, next)
{
    outputs = slots;
    assert(slots.size() > 1);
    mask = 0;
}

void Reader::OnRow(Entity *row)
{
    class Visitor : public Receiver
    {
    public:
        void OnRow(Entity *data) override
        {
            for (int i = 0; i < reader.outputs.size(); ++i)
                row[reader.outputs[i]] = data[i];
            reader.next->Evaluate(row);
        }

        Visitor(Reader &reader, Entity *row) : reader(reader), row(row) {}
        Reader &reader;
        Entity *row;
    };

    Visitor visitor{*this, row};
    relation->Query(nullptr, 0, visitor);
}

void Reader::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Read from ";
    OutputRelation(os, db, *relation);
    os << " into (";
    for (int i = 0; i < outputs.size(); ++i)
    {
        if (i > 0)
            os << ",";
        OutputVariable(os, outputs[i]);
    }
    os << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

Writer::Writer(Relation &relation, const std::vector<int> &slots) : WriterEvaluation(relation), slots(slots)
{
    assert(slots.size() > 0);
    slot = slots[0];
    contiguous = true;
    for (int i = 1; i < slots.size(); ++i)
        if (slots[i] != slot + i)
        {
            contiguous = false;
            break;
        }
}

void Writer::OnRow(Entity *row)
{
    if (contiguous)
        relation->Add(row + slot);
    else
    {
        // Assemble the data into a vector
        std::vector<Entity> data(slots.size());
        for (int i = 0; i < slots.size(); ++i)
            data[i] = row[slots[i]];
        relation->Add(&data[0]);
    }
}

void Writer::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Write (";
    for (int i = 0; i < slots.size(); ++i)
    {
        if (i > 0)
            os << ",";
        OutputVariable(os, slots[i]);
    }
    os << ") into ";
    OutputRelation(os, db, *relation);
    OutputCallCount(os);
    os << std::endl;
}

Join::Join(Relation &relation, const std::vector<int> &inputs, const std::vector<int> &outputs, const std::shared_ptr<Evaluation> &next) : ReaderEvaluation(relation, next)
{
    this->inputs = inputs;
    this->outputs = outputs;
    assert(inputs.size() == outputs.size());
    assert(relation.Arity() == inputs.size());
    mask = 0;
    int shift = 1;
    for (auto v : this->inputs)
    {
        if (v != -1)
            mask.mask |= shift;
        shift <<= 1;
    }
}

void Join::OnRow(Entity *locals)
{
    class Visitor : public Receiver
    {
    public:
        Visitor(Entity *locals, std::vector<int> &outputs, const std::shared_ptr<Evaluation> &next) : locals(locals), outputs(outputs), next(next)
        {
        }
        void OnRow(Entity *row) override
        {
            for (int i = 0; i < outputs.size(); ++i)
                if (outputs[i] != -1)
                    locals[outputs[i]] = row[i];
            next->Evaluate(locals);
        }
        Entity *locals;
        const std::vector<int> &outputs;
        std::shared_ptr<Evaluation> next;
    } visitor(locals, outputs, next);

    std::vector<Entity> data(inputs.size());
    for (int i = 0; i < inputs.size(); ++i)
        if (inputs[i] != -1)
        {
            data[i] = locals[inputs[i]];
        }

    if (useDelta)
        relation->QueryDelta(&data[0], mask, visitor);
    else
        relation->Query(&data[0], mask, visitor);
}

void Evaluation::OutputVariable(std::ostream &os, int variable)
{
    os << Colours::Variable << "_" << variable << Colours::Normal;
}

void Evaluation::OutputIntroducedVariable(std::ostream &os, int variable)
{
    os << Colours::IntroducedVariable << "_" << variable << Colours::Normal;
}

void Evaluation::OutputRelation(std::ostream &os, Database &db, const Relation &relation)
{
    os << Colours::Relation;
    if (auto cn = relation.GetCompoundName())
    {
        os << "has:";
        for (int i = 0; i < cn->parts.size(); ++i)
        {
            if (i > 0)
                os << ":";
            os << db.GetString(cn->parts[i]);
        }
    }
    else
    {
        if (relation.IsReaches())
            os << "reaches:";
        os << db.GetString(relation.Name());
    }

    switch (relation.GetBinding())
    {
    case BindingType::Binding:
    {
        auto cols = relation.GetBindingColumns();
        auto arity = relation.Arity();
        os << ":";
        for (int i = cols.mask; i; i>>=1)
            os << (i&1 ? "b" : "_");
    }
    break;
    case BindingType::Bound:
    {
        auto cols = relation.GetBindingColumns();
        auto arity = relation.Arity();
        os << ":";
        for (int i = cols.mask; i; i>>=1)
            os << (i&1 ? "B" : "_");
    }
    break;
    default: // Suppress warning
        break;
    }

    os << Colours::Normal;
}

void Join::Explain(Database &db, std::ostream &os, int indent) const
{
    int inCount = 0, outCount = 0;
    for (auto a : inputs)
        if (a != -1)
            ++inCount;
    for (auto a : outputs)
        if (a != -1)
            ++outCount;

    Indent(os, indent);
    if (inCount == 0)
        os << "Scan ";
    else if (outCount == 0)
        os << "Probe ";
    else
        os << "Join ";

    if (useDelta)
        os << Colours::Relation << "∆";
    OutputRelation(os, db, *relation);
    os << " (";
    for (int i = 0; i < inputs.size(); ++i)
    {
        if (i > 0)
            os << ",";
        if (inputs[i] != -1)
            OutputVariable(os, inputs[i]);
        else
            os << "_";
    }
    os << ") -> (";
    for (int i = 0; i < outputs.size(); ++i)
    {
        if (i > 0)
            os << ",";
        if (outputs[i] != -1)
            OutputIntroducedVariable(os, outputs[i]);
        else
            os << "_";
    }
    os << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

void Evaluation::SetGlobalCallCountLimit(std::size_t limit)
{
    globalCallCountLimit = limit;
}

std::size_t Evaluation::GetGlobalCallCountLimit()
{
    return globalCallCountLimit;
}

ReaderEvaluation::ReaderEvaluation(Relation &relation, const EvaluationPtr &next) : relation(&relation), ChainedEvaluation(next), mask(0)
{
}

WriterEvaluation::WriterEvaluation(Relation &relation) : relation(&relation)
{
}

ChainedEvaluation::ChainedEvaluation(const EvaluationPtr &next) : next(next)
{
}

OrEvaluationForNot::OrEvaluationForNot(const std::shared_ptr<Evaluation> &left, const std::shared_ptr<Evaluation> &right) : OrEvaluation(left, right)
{
}

std::shared_ptr<Evaluation> Evaluation::WithNext(const EvaluationPtr &next) const
{
    assert(0);
    return std::shared_ptr<Evaluation>();
}

std::shared_ptr<Evaluation> RuleEvaluation::WithNext(const std::shared_ptr<Evaluation> &next) const
{
    return std::make_shared<RuleEvaluation>(locals, next);
}

Deduplicate::Deduplicate(const EvaluationPtr &next) : ChainedEvaluation(next)
{
}

void DeduplicateB::Reset()
{
    values.clear();
}

void DeduplicateBB::Reset()
{
    values.clear();
}

DeduplicationGuard::DeduplicationGuard(const std::shared_ptr<Deduplicate> &dedup, const EvaluationPtr &next) : dedup(dedup), ChainedEvaluation(next)
{
}

void DeduplicationGuard::OnRow(Entity *row)
{
    next->Evaluate(row);
    dedup->Reset();
}

void DeduplicationGuard::Explain(Database &db, std::ostream &os, int indent) const
{
    next->Explain(db, os, indent);
}

CreateNew::CreateNew(Database &db, int slot, const std::shared_ptr<Evaluation> &next) : ChainedEvaluation(next), database(db), slot(slot)
{
}

void CreateNew::OnRow(Entity *row)
{
    row[slot] = database.NewEntity();
    next->Evaluate(row);
}

void CreateNew::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Create new ";
    OutputIntroducedVariable(os, slot);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

DeduplicateV::DeduplicateV(Database &db, const std::vector<int> &slots, const std::shared_ptr<Table> &table, const std::shared_ptr<Evaluation> &next) : Deduplicate(next),
                                                                                                                                                        database(db),
                                                                                                                                                        slots(slots),
                                                                                                                                                        table(table),
                                                                                                                                                        working(slots.size())
{
}

void DeduplicateV::OnRow(Entity *row)
{
    working.clear();
    for (auto i : slots)
        working.push_back(row[i]);
    if (table->Add(working.data()))
        next->Evaluate(row);
}

void DeduplicateV::Reset()
{
    table->Clear();
}

void DeduplicateV::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Deduplicate (";
    for (int i = 0; i < slots.size(); ++i)
    {
        if (i > 0)
            os << ",";
        OutputVariable(os, slots[i]);
    }
    os << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent + 4);
}

void Evaluation::VisitReads(const std::function<void(Relation *&, Columns, const int *)> &)
{
}

void Evaluation::VisitNext(const std::function<void(EvaluationPtr &, bool)> &)
{
}

void ChainedEvaluation::VisitNext(const std::function<void(EvaluationPtr &, bool)> &fn)
{
    fn(next, false);
}

void OrEvaluation::VisitNext(const std::function<void(EvaluationPtr &, bool)> &fn)
{
    fn(left, false);
    fn(right, false);
}

void OrEvaluationForNot::VisitNext(const std::function<void(EvaluationPtr &, bool)> &fn)
{
    fn(left, true);
    fn(right, false);
}

void ReaderEvaluation::VisitReads(const std::function<void(Relation *&relation, Columns, const int *)> &fn)
{
    fn(relation, mask, inputs.data());
}

void DeduplicateB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
}

void DeduplicateBB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
}

void OrEvaluation::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
}

void RuleEvaluation::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
}

void DeduplicationGuard::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
}

void Load::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Write);
}

void DeduplicateV::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    for (auto &v : slots)
        fn(v, VariableAccess::Read);
}

void SumCollector::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Read);
    fn(sumSlot, VariableAccess::ReadWrite);
}

void NoneEvaluation::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
}

void NotInB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Read);
}

void NotTerminator::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Write);
}

void NotNone::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Read);
}

void CreateNew::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Write);
}

void CountCollector::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot, VariableAccess::Write);
}

void ReaderEvaluation::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    for (auto &i : inputs)
        if (i != -1)
            fn(i, VariableAccess::Read);

    for (auto &i : outputs)
        if (i != -1)
            fn(i, VariableAccess::Write);
}

void AddBBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Write);
}

void SubBBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Write);
}

void MulBBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Write);
}

void DivBBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Write);
}

void ModBBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Write);
}

void RangeB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
    fn(slot3, VariableAccess::Read);
}

void RangeU::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Write);
    fn(slot3, VariableAccess::Read);
}

void Writer::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    for (auto &i : slots)
        fn(i, VariableAccess::Read);
}

void EqualsBB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
}

void EqualsBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Write);
}

void CompareBB::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Read);
}

void NegateBF::VisitVariables(const std::function<void(int &, VariableAccess)> &fn)
{
    fn(slot1, VariableAccess::Read);
    fn(slot2, VariableAccess::Write);
}

void Evaluation::VisitWrites(const std::function<void(Relation *&rel, int, const int *)> &fn)
{
}

void Writer::VisitWrites(const std::function<void(Relation *&rel, int, const int *)> &fn)
{
    fn(relation, slots.size(), slots.data());
}

EvaluationPtr DeduplicateB::Clone() const
{
    return cloneHelper = std::make_shared<DeduplicateB>(slot1, next->Clone());
}

EvaluationPtr DeduplicateV::Clone() const
{
    return cloneHelper = std::make_shared<DeduplicateV>(database, slots, table, next->Clone());
}

EvaluationPtr OrEvaluation::Clone() const
{
    return std::make_shared<OrEvaluation>(left->Clone(), right->Clone());
}

EvaluationPtr SumCollector::Clone() const
{
    return std::make_shared<SumCollector>(slot, sumSlot);
}

EvaluationPtr DeduplicateBB::Clone() const
{
    return cloneHelper = std::make_shared<DeduplicateBB>(slot1, slot2, next->Clone());
}

EvaluationPtr NotTerminator::Clone() const
{
    return std::make_shared<NotTerminator>(slot);
}

EvaluationPtr NoneEvaluation::Clone() const
{
    return std::make_shared<NoneEvaluation>();
}

EvaluationPtr CountCollector::Clone() const
{
    return std::make_shared<CountCollector>(slot);
}

EvaluationPtr RuleEvaluation::Clone() const
{
    return std::make_shared<RuleEvaluation>(locals, next->Clone());
}

EvaluationPtr OrEvaluationForNot::Clone() const
{
    return std::make_shared<OrEvaluationForNot>(left->Clone(), right->Clone());
}

EvaluationPtr Join::Clone() const
{
    return std::make_shared<Join>(*relation, inputs, outputs, next->Clone());
}

EvaluationPtr Load::Clone() const
{
    return std::make_shared<Load>(slot, value, next->Clone());
}

EvaluationPtr AddBBF::Clone() const
{
    return std::make_shared<AddBBF>(database, slot1, slot2, slot3, next->Clone());
}

EvaluationPtr SubBBF::Clone() const
{
    return std::make_shared<SubBBF>(slot1, slot2, slot3, next->Clone());
}

EvaluationPtr MulBBF::Clone() const
{
    return std::make_shared<MulBBF>(slot1, slot2, slot3, next->Clone());
}

EvaluationPtr DivBBF::Clone() const
{
    return std::make_shared<DivBBF>(slot1, slot2, slot3, next->Clone());
}

EvaluationPtr ModBBF::Clone() const
{
    return std::make_shared<ModBBF>(slot1, slot2, slot3, next->Clone());
}

EvaluationPtr NotInB::Clone() const
{
    return std::make_shared<NotInB>(slot, *relation, next->Clone());
}

EvaluationPtr RangeB::Clone() const
{
    return std::make_shared<RangeB>(slot1, cmp1, slot2, cmp2, slot3, next->Clone());
}

EvaluationPtr RangeU::Clone() const
{
    return std::make_shared<RangeU>(slot1, cmp1, slot2, cmp2, slot3, next->Clone());
}

EvaluationPtr EqualsBB::Clone() const
{
    return std::make_shared<EqualsBB>(slot1, slot2, next->Clone());
}

EvaluationPtr EqualsBF::Clone() const
{
    return std::make_shared<EqualsBF>(slot1, slot2, next->Clone());
}

EvaluationPtr CreateNew::Clone() const
{
    return std::make_shared<CreateNew>(database, slot, next->Clone());
}

EvaluationPtr NotNone::Clone() const
{
    return std::make_shared<NotNone>(slot, next->Clone());
}

EvaluationPtr CompareBB::Clone() const
{
    return std::make_shared<CompareBB>(slot1, cmp, slot2, next->Clone());
}

EvaluationPtr NegateBF::Clone() const
{
    return std::make_shared<NegateBF>(slot1, slot2, next->Clone());
}

EvaluationPtr Reader::Clone() const
{
    return std::make_shared<Reader>(*relation, outputs, next->Clone());
}

EvaluationPtr Writer::Clone() const
{
    return std::make_shared<Writer>(*relation, slots);
}

EvaluationPtr DeduplicationGuard::Clone() const
{
    // next->Clone will assign cloneHelper
    auto n = next->Clone();
    assert(dedup->cloneHelper);
    return std::make_shared<DeduplicationGuard>(dedup->cloneHelper, n);
}
