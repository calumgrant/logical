#include "Evaluation.hpp"
#include "Database.hpp"
#include "AST.hpp"
#include "Colours.hpp"

#include <iostream>
#include <sstream>

Evaluation::~Evaluation()
{
}

NoneEvaluation::NoneEvaluation()
{
    // This is useful for a breakpoint
}

void NoneEvaluation::Evaluate(Entity * row)
{
    IncrementCallCount();
}

WriterB::WriterB(const std::shared_ptr<Relation> & relation, int slot) : relation(relation), slot(slot)
{
}

void WriterB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;

    relation.lock()->Add(row+slot);
}


OrEvaluation::OrEvaluation(const std::shared_ptr<Evaluation> & lhs, const std::shared_ptr<Evaluation> & rhs) :
    left(lhs), right(rhs)
{
}

void OrEvaluation::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    left->Evaluate(row);
    right->Evaluate(row);
}

RuleEvaluation::RuleEvaluation(int locals, const std::shared_ptr<Evaluation> & eval) :
    locals(locals), evaluation(eval)
{
}

void RuleEvaluation::Evaluate(Entity*)
{
    if(IncrementCallCount()) return;
    std::vector<Entity> row(locals);
    evaluation->Evaluate(&row[0]);
}

EvaluateB::EvaluateB(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next) :
    UnaryEvaluation(rel, slot, next)
{
}

EvaluateF::EvaluateF(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next) :
    UnaryEvaluation(rel, slot, next)
{
}

UnaryEvaluation::UnaryEvaluation(const std::shared_ptr<Relation> &rel, int slot, const std::shared_ptr<Evaluation> &next) :
                     relation(rel), slot(slot), next(next)
{
}

class UnaryVisitor : public Relation::Visitor
{
public:
    UnaryVisitor(Entity * row, int slot, Evaluation & next) :
        row(row), slot(slot), next(next)
    {
    }
    
    void OnRow(const Entity *e) override
    {
        row[slot] = e[0];
        next.Evaluate(row);
    }
    
    Entity * row;
    int slot;
    Evaluation & next;
};

void EvaluateF::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    UnaryVisitor v(row, slot, *next);
    
    relation.lock()->Query(row+slot, 0, v);
}

void EvaluateB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    UnaryVisitor v(row, slot, *next);

    // Bug: We need to get a different relation here.
    relation.lock()->Query(row+slot, 1, v);
}

void OrEvaluation::Explain(Database &db, std::ostream & os, int indent) const
{
    left->Explain(db, os, indent);
    right->Explain(db, os, indent);
}

void NoneEvaluation::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "None";
    OutputCallCount(os);
    os << "\n";
}

void Evaluation::Indent(std::ostream & os, int indent)
{
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
}

void RuleEvaluation::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Evaluate with " << locals << " variables";
    OutputCallCount(os);
    os << " ->\n";
    evaluation->Explain(db, os, indent+4);
}

void WriterB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Write ";
    OutputVariable(os, slot);
    os << " into ";
    OutputRelation(os, db, relation.lock());
    OutputCallCount(os);
    os << "\n";
}

void EvaluateB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Lookup ";
    OutputVariable(os, slot);
    os << " in ";
    OutputRelation(os, db, r);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

void EvaluateF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Scan ";
    OutputRelation(os, db, r);
    os << " into ";
    OutputVariable(os, slot);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

NotTerminator::NotTerminator(int slot) : slot(slot)
{
}

void NotTerminator::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    row[slot].type = EntityType::None;
}

void NotTerminator::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Load ";
    OutputVariable(os, slot);
    os << " := ";
    db.PrintQuoted(Entity(), os);
    OutputCallCount(os);
    os << "\n";
}

BinaryEvaluation::BinaryEvaluation(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), next(next)
{
}

EqualsBB::EqualsBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next)
{
}

void EqualsBB::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    if(row[slot1] == row[slot2])
        next->Evaluate(row);
}

void EqualsBB::Explain(Database & db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " == _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

EqualsBF::EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next)
{
}

void EqualsBF::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    row[slot2] = row[slot1];
    next->Evaluate(row);
}

void EqualsBF::Explain(Database & db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Assign ";
    OutputVariable(os, slot2);
    os << " := ";
    OutputVariable(os, slot1);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateBB::EvaluateBB(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateBB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(const std::shared_ptr<Evaluation> & next, Entity * row) : row(row), next(next)
        {
        }
        
        void OnRow(const Entity * data) override
        {
            next->Evaluate(row);
        }
    private:
        Entity * row;
        std::shared_ptr<Evaluation> next;
    } v(next, row);
    
    Entity data[2] = { row[slot1], row[slot2] };

    relation.lock()->Query(data, 3, v);
}

void EvaluateBB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Lookup (_" << slot1 << ",_" << slot2 << ") in " << db.GetString(relation.lock()->Name());
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateBF::EvaluateBF(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateBF::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(Entity * row, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
            row(row), slot1(slot1), slot2(slot2), next(next)
        {
        }
        
        void OnRow(const Entity * data) override
        {
            row[slot2] = data[1];
            next->Evaluate(row);
        }
    private:
        Entity * row;
        int slot1, slot2;
        std::shared_ptr<Evaluation> next;
    } visitor(row, slot1, slot2, next);
    
    Entity data[2] = { row[slot1] };
    
    relation.lock()->Query(data, 1, visitor);
}

void EvaluateBF::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Join " << db.GetString(relation.lock()->Name()) << " column 1 on _" << slot1 << " into _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    
    next->Explain(db, os, indent+4);
}

EvaluateFB::EvaluateFB(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateFB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(Entity * row, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
            row(row), slot1(slot1), slot2(slot2), next(next)
        {
        }
        
        void OnRow(const Entity * data) override
        {
            row[slot1] = data[0];
            next->Evaluate(row);
        }
    private:
        Entity * row;
        int slot1, slot2;
        std::shared_ptr<Evaluation> next;
    } visitor(row, slot1, slot2, next);
    
    Entity data[2];
    data[1] = row[slot2];
    
    relation.lock()->Query(data, 2, visitor);
}

void EvaluateFB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Join " << db.GetString(relation.lock()->Name()) << " column 2 on _" << slot2 << " into _" << slot1;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateFF::EvaluateFF(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateFF::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(Entity * row, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
            row(row), slot1(slot1), slot2(slot2), next(next)
        {
        }
        
        void OnRow(const Entity * data) override
        {
            row[slot1] = data[0];
            row[slot2] = data[1];
            next->Evaluate(row);
        }
    private:
        Entity * row;
        int slot1, slot2;
        std::shared_ptr<Evaluation> next;
    } visitor(row, slot1, slot2, next);
    
    relation.lock()->Query(nullptr, 0, visitor);
}

void EvaluateFF::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Scan " << db.GetString(relation.lock()->Name()) << " into (_" << slot1 << ",_" << slot2 << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

BinaryRelationEvaluation::BinaryRelationEvaluation(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next),
    relation(relation)
{
}

WriterBB::WriterBB(const std::shared_ptr<Relation> & relation, int slot1, int slot2) :
    relation(relation), slot1(slot1), slot2(slot2)
{
}

void WriterBB::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    Entity data[2] = { row[slot1], row[slot2] };
    relation.lock()->Add(data);
}

void WriterBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Write (_" << slot1 << ",_" << slot2 << ") into " << db.GetString(relation.lock()->Name());
    OutputCallCount(os);
    os << std::endl;
}

RangeB::RangeB(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), slot3(slot3), cmp1(cmp1), cmp2(cmp2), next(next)
{
}

template<typename T>
bool Compare(T value1, T value2, ComparatorType t)
{
    switch(t)
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
    if(e1.type == EntityType::Integer && e2.type == EntityType::Integer)
        return Compare(e1.i, e2.i, cmp);
    else if(e1.type ==EntityType::Integer && e2.type == EntityType::Float)
        return Compare<float>(e1.i, e2.f, cmp);
    else if(e1.type ==EntityType::Float && e2.type == EntityType::Integer)
        return Compare<float>(e1.f, e2.i, cmp);
    else if(e1.type ==EntityType::Float && e2.type == EntityType::Float)
        return Compare(e1.f, e2.f, cmp);
    else
        return false;
}

void RangeB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    if(Compare(row[slot1], row[slot2], cmp1) && Compare(row[slot2], row[slot3], cmp2))
        next->Evaluate(row);
}

void RangeB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " " << cmp1 << " _" << slot2 << " and _" << slot2 << " " << cmp2 << " _" << slot3;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

RangeU::RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), slot3(slot3), cmp1(cmp1), cmp2(cmp2), next(next)
{
}

void RangeU::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    int lowerBound, upperBound;
    
    if(row[slot1].type == EntityType::Integer)
    {
        lowerBound = cmp1 == ComparatorType::lt ? row[slot1].i+1 : row[slot1].i;
    }
    else
        return;  // For now, floats are not supported

    if(row[slot3].type == EntityType::Integer)
    {
        upperBound = cmp2 == ComparatorType::lt ? row[slot3].i-1 : row[slot3].i;
    }
    else
        return;  // For now, floats are not supported
    
    row[slot2].type = EntityType::Integer;
    int & value = row[slot2].i;
    for(value=lowerBound; value<=upperBound; ++value)
    {
        next->Evaluate(row);
    }
}

void RangeU::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "For _" << slot1 << " " << cmp1 << " _" << slot2 << " " << cmp2 << " _" << slot3;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

CompareBB::CompareBB(int slot1, ComparatorType cmp, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next), cmp(cmp)
{
}

void CompareBB::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    // Note: We don't compare strings for inequality.
    // FIXME
    if(Compare(row[slot1], row[slot2], cmp))
        next->Evaluate(row);
}

void CompareBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << cmp << "_" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

NegateBF::NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next)
{
}

void NegateBF::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    switch(row[slot1].type)
    {
    case EntityType::Integer:
        row[slot2].type = EntityType::Integer;
        row[slot2].i = - row[slot1].i;
        break;
    case EntityType::Float:
        row[slot2].type = EntityType::Integer;
        row[slot2].f = - row[slot1].f;
        break;
    default:
        return;  // Fail
    }
    next->Evaluate(row);
}

void NegateBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot2 << " := -_" << slot1;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

BinaryArithmeticEvaluation::BinaryArithmeticEvaluation(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), slot3(slot3), next(next)
{
}

AddBBF::AddBBF(Database &db, int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    BinaryArithmeticEvaluation(slot1, slot2, slot3, next), database(db)
{
}

SubBBF::SubBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

MulBBF::MulBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

DivBBF::DivBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

ModBBF::ModBBF(int slot1, int slot2, int slot3, const std::shared_ptr<Evaluation> & next) :
    BinaryArithmeticEvaluation(slot1, slot2, slot3, next)
{
}

void AddBBF::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    auto t1 = row[slot1].type;
    auto t2 = row[slot2].type;
    
    if(t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Integer;
        row[slot3].i = row[slot1].i + row[slot2].i;
    }
    else if(t1 == EntityType::String && t2 == EntityType::String)
    {
        row[slot3] = database.AddStrings(row[slot1].i,row[slot2].i);
    }
    else if(t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = row[slot1].f + row[slot2].f;
    }
    else if(t1 == EntityType::String || t2 == EntityType::String)
    {
        // Convert the other to a string
        if(t1 == EntityType::Integer)
        {
            std::ostringstream ss;
            ss << row[slot1].i << database.GetString(row[slot2].i);
            row[slot3] = database.CreateString(ss.str());
        }
        else if(t1 == EntityType::Float)
        {
            std::ostringstream ss;
            ss << row[slot1].f << database.GetString(row[slot2].i);
            row[slot3] = database.CreateString(ss.str());

        }
        else if(t2 == EntityType::Integer)
        {
            std::ostringstream ss;
            ss << database.GetString(row[slot1].i) << row[slot2].i;
            row[slot3] = database.CreateString(ss.str());
        }
        else if(t2 == EntityType::Float)
        {
            std::ostringstream ss;
            ss << database.GetString(row[slot1].i) << row[slot2].f;
            row[slot3] = database.CreateString(ss.str());
        }
        else
            return;
    }
    else if(t1 == EntityType::Float || t2 == EntityType::Float)
    {
        // Convert the other to a float
        if(t1 == EntityType::Integer)
        {
            row[slot3].type = EntityType::Float;
            row[slot3].f = row[slot1].i + row[slot2].f;
        }
        else if(t2 == EntityType::Integer)
        {
            row[slot3].type = EntityType::Float;
            row[slot3].f = row[slot1].f + row[slot2].i;
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
    os << "Calculate _" << slot3 << " := _" << slot1 << " + _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

template<typename OpInt, typename OpFloat>
void BinaryArithmeticEvaluation::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;

    auto t1 = row[slot1].type;
    auto t2 = row[slot2].type;
    
    if(t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Integer;
        row[slot3].i = OpInt()(row[slot1].i,row[slot2].i);
    }
    else if(t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = OpFloat()(row[slot1].f,row[slot2].f);
    }
    else if(t1 == EntityType::Float && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = OpFloat()(row[slot1].f, row[slot2].i);
    }
    else if(t1 == EntityType::Integer && t2 == EntityType::Float)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = OpFloat()(row[slot1].i, row[slot2].f);
    }
    else
        return;
    
    next->Evaluate(row);
}

void SubBBF::Evaluate(Entity *row)
{
    BinaryArithmeticEvaluation::Evaluate<std::minus<int>, std::minus<float>>(row);
}

void SubBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate ";
    OutputVariable(os, slot3);
    os << " := ";
    OutputVariable(os, slot1);
    os << " - ";
    OutputVariable(os, slot2);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

void MulBBF::Evaluate(Entity *row)
{
    BinaryArithmeticEvaluation::Evaluate<std::multiplies<int>, std::multiplies<float>>(row);
}

void MulBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " * _" << slot2;
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

void Evaluation::OutputCallCount(std::ostream & os) const
{
    if(callCount==1)
        os << " (called " << callCount << " time)";
    else if(callCount>1)
        os << " (called " << callCount << " times)";
}

void DivBBF::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    auto t1 = row[slot1].type;
    auto t2 = row[slot2].type;
    
    if(t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Integer;
        if(row[slot2].i == 0) return;
        row[slot3].i = row[slot1].i / row[slot2].i;
    }
    else if(t1 == EntityType::Float && t2 == EntityType::Float)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = row[slot1].f / row[slot2].f;
    }
    else if(t1 == EntityType::Float && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = row[slot1].f / row[slot2].i;
    }
    else if(t1 == EntityType::Integer && t2 == EntityType::Float)
    {
        row[slot3].type = EntityType::Float;
        row[slot3].f = row[slot1].i / row[slot2].f;
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
    next->Explain(db, os, indent+4);
}

void ModBBF::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    auto t1 = row[slot1].type;
    auto t2 = row[slot1].type;
    
    if(t1 == EntityType::Integer && t2 == EntityType::Integer)
    {
        row[slot3].type = EntityType::Integer;
        if(row[slot2].i == 0) return;
        row[slot3].i = row[slot1].i % row[slot2].i;
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
    next->Explain(db, os, indent+4);
}

Evaluation::Evaluation() : callCount(0)
{
}

DeduplicateB::DeduplicateB(int slot1, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), next(next)
{
}

void DeduplicateB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;

    auto i = values.insert(row[slot1]);
    if(i.second)
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
    next->Explain(db, os, indent+4);
}

DeduplicateBB::DeduplicateBB(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), next(next)
{
}

void DeduplicateBB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;

    auto i = values.insert(std::make_pair(row[slot1], row[slot2]));
    if(i.second)
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
    next->Explain(db, os, indent+4);
}


CountCollector::CountCollector(int slot) : slot(slot)
{
}

void CountCollector::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    row[slot].type = EntityType::Integer;
    ++row[slot].i;
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

void SumCollector::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    row[sumSlot] += row[slot];
}

void SumCollector::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Sum _" << slot << " into _" << sumSlot;
    OutputCallCount(os);
    std::cout << std::endl;
}

Load::Load(int slot, const Entity &v, const std::shared_ptr<Evaluation> & next) :
    slot(slot), value(v), next(next)
{
}

void Load::Evaluate(Entity * locals)
{
    if(IncrementCallCount()) return;
    locals[slot] = value;
    next->Evaluate(locals);
}

void Load::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Load ";
    OutputVariable(os, slot);
    os << " := ";
    db.PrintQuoted(value, os);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

NotNone::NotNone(int slot, const std::shared_ptr<Evaluation> & next) : slot(slot), next(next)
{
}

void NotNone::Evaluate(Entity *row)
{
    if(IncrementCallCount()) return;
    if(row[slot].type != EntityType::None)
        next->Evaluate(row);
}

void NotNone::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Check ";
    OutputVariable(os, slot);
    os << " is not ";
    db.PrintQuoted(Entity(), os);
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

NotInB::NotInB(int slot, const std::shared_ptr<Relation> & relation, const std::shared_ptr<Evaluation> & next) :
    slot(slot), relation(relation), next(next)
{
}

void NotInB::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    class Visitor : public Relation::Visitor
    {
    public:
        bool found = false;
        void OnRow(const Entity *) override { found = true; }
    } visitor;
    relation.lock()->Query(row+slot, 1, visitor);
    
    if(!visitor.found) next->Evaluate(row);
}

void NotInB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Lookup _" << slot << " is not in " << db.GetString(relation.lock()->Name());
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

std::size_t Evaluation::globalCallCount = 0;
std::size_t Evaluation::globalCallCountLimit = -1;

std::size_t Evaluation::GlobalCallCount()
{
    return globalCallCount;
}

Reader::Reader(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots, const std::shared_ptr<Evaluation> & next) :
    relation(relation), slots(slots), next(next)
{
    assert(slots.size()>1);
}

void Reader::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;
    
    class Visitor : public Relation::Visitor
    {
    public:
        void OnRow(const Entity * data) override
        {
            for(int i=0; i<reader.slots.size(); ++i)
                row[reader.slots[i]] = data[i];
            reader.next->Evaluate(row);
        }
        
        Visitor(Reader & reader, Entity * row) : reader(reader), row(row) { }
        Reader & reader;
        Entity * row;
    };
    
    Visitor visitor { *this, row };
    relation.lock()->Query(nullptr, 0, visitor);
}

void Reader::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Read from ";
    OutputRelation(os, db, relation.lock());
    os << " into (";
    for(int i=0; i<slots.size(); ++i)
    {
        if(i>0) os << ",";
        OutputVariable(os, slots[i]);
    }
    os << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}

Writer::Writer(const std::shared_ptr<Relation> & relation, const std::vector<int> & slots) :
    relation(relation), slots(slots)
{
    assert(slots.size()>0);
    slot = slots[0];
    contiguous = true;
    for(int i=1; i<slots.size(); ++i)
        if(slots[i] != slot+i) { contiguous = false; break; }
}

void Writer::Evaluate(Entity * row)
{
    if(IncrementCallCount()) return;

    if(contiguous)
        relation.lock()->Add(row + slot);
    else
    {
        // Assemble the data into a vector
        std::vector<Entity> data(slots.size());
        for(int i=0; i<slots.size(); ++i)
            data[i] = row[slots[i]];
        relation.lock()->Add(&data[0]);
    }
}

void Writer::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Write (";
    for(int i=0; i<slots.size(); ++i)
    {
        if(i>0) os << ",";
        OutputVariable(os, slots[i]);
    }
    os << ") into ";
    OutputRelation(os, db, relation.lock());
    OutputCallCount(os);
    os << std::endl;
}

Join::Join(const std::shared_ptr<Relation> & relation, std::vector<int> && inputs, std::vector<int> && outputs, const std::shared_ptr<Evaluation> & next) : relation(relation), inputs(inputs), outputs(outputs), next(next)
{
    assert(inputs.size() == outputs.size());
    assert(relation->Arity() == inputs.size());
    mask = 0;
    int shift = 1;
    for(auto v : this->inputs)
    {
        if(v != -1) mask |= shift;
        shift <<= 1;
    }
}

void Join::Evaluate(Entity * locals)
{
    if(IncrementCallCount()) return;

    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(Entity * locals, std::vector<int> & outputs, const std::shared_ptr<Evaluation> & next) :
            locals(locals), outputs(outputs), next(next)
        {
        }
        void OnRow(const Entity * row) override
        {
            for(int i=0; i<outputs.size(); ++i)
                if(outputs[i] != -1)
                    locals[outputs[i]] = row[i];
            next->Evaluate(locals);
        }
        Entity * locals;
        const std::vector<int> & outputs;
        std::shared_ptr<Evaluation> next;
    } visitor(locals, outputs, next);
    
    std::vector<Entity> data(inputs.size());
    for(int i=0; i<inputs.size(); ++i)
        if(inputs[i] != -1)
        {
            data[i] = locals[inputs[i]];
        }
    
    relation.lock()->Query(&data[0], mask, visitor);
}

void Evaluation::OutputVariable(std::ostream & os, int variable)
{
    os << Colours::Variable << "_" << variable << Colours::Normal;
}

void Evaluation::OutputRelation(std::ostream &os, Database &db, const std::shared_ptr<Relation> & relation)
{
    os << Colours::Relation;
    if(auto cn = relation->GetCompoundName())
    {
        os << "has:";
        for(int i=0; i<cn->parts.size(); ++i)
        {
            if(i>0) os << ":";
            os << db.GetString(cn->parts[i]);
        }
    }
    else
        os << db.GetString(relation->Name());
    os << Colours::Normal;
}

void Join::Explain(Database &db, std::ostream & os, int indent) const
{
    int inCount=0, outCount=0;
    for(auto a : inputs) if(a!=-1) ++ inCount;
    for(auto a : outputs) if(a!=-1) ++ outCount;
    
    Indent(os, indent);
    if(inCount==0) os << "Scan ";
    else if(outCount==0) os << "Probe ";
    else os << "Join ";
    
    OutputRelation(os, db, relation.lock());
    os << " (";
    for(int i=0; i<inputs.size(); ++i)
    {
        if(i>0) os << ",";
        if(inputs[i] != -1)
            OutputVariable(os, inputs[i]);
        else
            os << "_";
    }
    os << ") -> (";
    for(int i=0; i<outputs.size(); ++i)
    {
        if(i>0) os << ",";
        if(outputs[i] != -1)
            OutputVariable(os, outputs[i]);
        else
            os << "_";
    }
    os << ")";
    OutputCallCount(os);
    os << " ->\n";
    next->Explain(db, os, indent+4);
}
