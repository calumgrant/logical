#include "Evaluation.hpp"
#include "Database.hpp"
#include "AST.hpp"

#include <iostream>

Evaluation::~Evaluation()
{
}

NoneEvaluation::NoneEvaluation()
{
    // This is useful for a breakpoint
}

void NoneEvaluation::Evaluate(Entity * row)
{
}

WriterB::WriterB(const std::shared_ptr<Relation> & relation, int slot) : relation(relation), slot(slot)
{
}

void WriterB::Evaluate(Entity * row)
{
    relation->Add(row+slot);
}


OrEvaluation::OrEvaluation(const std::shared_ptr<Evaluation> & lhs, const std::shared_ptr<Evaluation> & rhs) :
    left(lhs), right(rhs)
{
}

void OrEvaluation::Evaluate(Entity * row)
{
    left->Evaluate(row);
    right->Evaluate(row);
}

RuleEvaluation::RuleEvaluation(std::vector<Entity> &&row, const std::shared_ptr<Evaluation> & eval) :
    row(row), evaluation(eval), evaluated(false)
{
}

void RuleEvaluation::Evaluate(Entity*)
{
    if(!evaluated)
    {
        evaluated = true;
        evaluation->Evaluate(&row[0]);
    }
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
    UnaryVisitor v(row, slot, *next);
    
    relation.lock()->Query(row+slot, 0, v);
}

void EvaluateB::Evaluate(Entity * row)
{
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
    os << "None\n";
}

void Evaluation::Indent(std::ostream & os, int indent)
{
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
}

void RuleEvaluation::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Evaluate with " << row.size() << " variables ->\n";
    for(int i=0; i<row.size(); ++i)
    {
        if(row[i].type != EntityType::None)
        {
            Indent(os, indent+4);
            os << "_" << i << " = ";
            db.PrintQuoted(row[i], os);
            os << std::endl;
        }
    }
    evaluation->Explain(db, os, indent+4);
}

void WriterB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Write _" << slot << " into " << relation->Name() << "\n";
}

void EvaluateB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Lookup _" << slot << " in " << r->Name() << " ->\n";
    next->Explain(db, os, indent+4);
}

void EvaluateF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Scan " << r->Name() << " into _" << slot << " ->\n";
    next->Explain(db, os, indent+4);
}

void NotTerminator::Evaluate(Entity * row)
{
    resultFound = true;
}

void NotTerminator::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Fail next else\n";
}

NotEvaluation::NotEvaluation(const std::shared_ptr<NotTerminator> & terminator, const std::shared_ptr<Evaluation> &notBody, const std::shared_ptr<Evaluation> & next) : terminator(terminator), notBody(notBody), next(next)
{
}

void NotEvaluation::Evaluate(Entity * row)
{
    terminator->resultFound = false;
    notBody->Evaluate(row);
    if( !terminator->resultFound )
        next->Evaluate(row);
}

void NotEvaluation::Explain(Database &db, std::ostream & os, int indent) const
{
    notBody->Explain(db, os, indent);

    Indent(os, indent);
    os << "Else ->\n";
    next->Explain(db, os, indent+4);
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
    if(row[slot1] == row[slot2])
        next->Evaluate(row);
}

void EqualsBB::Explain(Database & db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " == _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

EqualsBF::EqualsBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next)
{
}

void EqualsBF::Evaluate(Entity *row)
{
    row[slot2] = row[slot1];
    next->Evaluate(row);
}

void EqualsBF::Explain(Database & db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Assign _" << slot2 << " := _" << slot1 << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateBB::EvaluateBB(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateBB::Evaluate(Entity * row)
{
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
    os << "Lookup (_" << slot1 << ",_" << slot2 << ") in " << relation.lock()->Name() << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateBF::EvaluateBF(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateBF::Evaluate(Entity * row)
{
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
    os << "Join " << relation.lock()->Name() << " column 1 on _" << slot1 << " into _" << slot2 << " ->\n";
    
    next->Explain(db, os, indent+4);
}

EvaluateFB::EvaluateFB(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateFB::Evaluate(Entity * row)
{
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
    os << "Join " << relation.lock()->Name() << " column 2 on _" << slot2 << " into _" << slot1 << " ->\n";
    next->Explain(db, os, indent+4);
}

EvaluateFF::EvaluateFF(const std::shared_ptr<Relation> & relation, int slot1, int slot2, const std::shared_ptr<Evaluation> & next) : BinaryRelationEvaluation(relation, slot1, slot2, next)
{
}

void EvaluateFF::Evaluate(Entity * row)
{
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
    os << "Scan " << relation.lock()->Name() << " into (_" << slot1 << ",_" << slot2 << ") ->\n";
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
    Entity data[2] = { row[slot1], row[slot2] };
    relation.lock()->Add(data);
}

void WriterBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Write (_" << slot1 << ",_" << slot2 << ") into " << relation.lock()->Name() << std::endl;
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
    if(Compare(row[slot1], row[slot2], cmp1) && Compare(row[slot2], row[slot3], cmp2))
        next->Evaluate(row);
}

void RangeB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << " " << cmp1 << " _" << slot2 << " and _" << slot2 << " " << cmp2 << " _" << slot3 << " ->\n";
    next->Explain(db, os, indent+4);
}

RangeU::RangeU(int slot1, ComparatorType cmp1, int slot2, ComparatorType cmp2, int slot3, const std::shared_ptr<Evaluation> & next) :
    slot1(slot1), slot2(slot2), slot3(slot3), cmp1(cmp1), cmp2(cmp2), next(next)
{
}

void RangeU::Evaluate(Entity * row)
{
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
    os << "Scan _" << slot1 << " " << cmp1 << " _" << slot2 << " and _" << slot2 << " " << cmp2 << " _" << slot3 << " ->\n";
    next->Explain(db, os, indent+4);
}

CompareBB::CompareBB(int slot1, ComparatorType cmp, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next), cmp(cmp)
{
}

void CompareBB::Evaluate(Entity *row)
{
    // Note: We don't compare strings.
    // FIXME
    if(Compare(row[slot1], row[slot2], cmp))
        next->Evaluate(row);
}

void CompareBB::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Test _" << slot1 << cmp << "_" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

NegateBF::NegateBF(int slot1, int slot2, const std::shared_ptr<Evaluation> & next) :
    BinaryEvaluation(slot1, slot2, next)
{
}

void NegateBF::Evaluate(Entity *row)
{
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
    os << "Calculate _" << slot2 << " := -_" << slot1 << " ->\n";
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
    if(row[slot1].type == EntityType::Integer && row[slot2].type == EntityType::Integer)
    {
        row[slot3].type = EntityType::Integer;
        row[slot3].i = row[slot1].i + row[slot2].i;
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
    os << "Calculate _" << slot3 << " := _" << slot1 << " + _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

void SubBBF::Evaluate(Entity *row)
{
    next->Evaluate(row);
}

void SubBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " - _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

void MulBBF::Evaluate(Entity *row)
{
    next->Evaluate(row);
}

void MulBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " * _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

void DivBBF::Evaluate(Entity *row)
{
    next->Evaluate(row);
}

void DivBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " / _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}

void ModBBF::Evaluate(Entity *row)
{
    next->Evaluate(row);
}

void ModBBF::Explain(Database &db, std::ostream &os, int indent) const
{
    Indent(os, indent);
    os << "Calculate _" << slot3 << " := _" << slot1 << " % _" << slot2 << " ->\n";
    next->Explain(db, os, indent+4);
}
