#include "Evaluation.hpp"
#include "Database.hpp"

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
}

void EvaluateFB::Explain(Database &db, std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Join (out _" << slot1 << ", in _" << slot2 << ") in " << relation.lock()->Name() << " ->\n";
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
