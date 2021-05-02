#include "Evaluation.hpp"
#include <iostream>

Evaluation::~Evaluation()
{
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
    row(row), evaluation(eval)
{
}

void RuleEvaluation::Evaluate(Entity*)
{
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
    UnaryVisitor v(row, slot, *next);
    
    relation.lock()->Query(row+slot, 0, v);
}

void EvaluateB::Evaluate(Entity * row)
{
    UnaryVisitor v(row, slot, *next);

    // Bug: We need to get a different relation here.
    relation.lock()->Query(row+slot, 1, v);
}

void OrEvaluation::Explain(std::ostream & os, int indent) const
{
    left->Explain(os, indent);
    right->Explain(os, indent);
}

void NoneEvaluation::Explain(std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "None\n";
}

void Evaluation::Indent(std::ostream & os, int indent)
{
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
}

void RuleEvaluation::Explain(std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Rule with " << row.size() << " variables ->\n";
    evaluation->Explain(os, indent+4);
}

void WriterB::Explain(std::ostream & os, int indent) const
{
    Indent(os, indent);
    os << "Write _" << slot << " into " << relation->Name() << "\n";
}

void EvaluateB::Explain(std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Lookup _" << slot << " in " << r->Name() << " ->\n";
    next->Explain(os, indent+4);
}

void EvaluateF::Explain(std::ostream &os, int indent) const
{
    Indent(os, indent);
    auto r = relation.lock();
    assert(r);
    os << "Scan " << r->Name() << " into _" << slot << " ->\n";
    next->Explain(os, indent+4);
}
