#include "Compiler.hpp"
#include "Database.hpp"
#include "Evaluation.hpp"

#include <iostream>

// Deleteme?
class VariableNumberer : public AST::Visitor
{
public:
    std::unordered_map<std::string, int> variables;
    void OnVariable(const std::string &name) override
    {
        variables.insert(std::make_pair(name, variables.size()));
    }
};

class LeftHandSide : public AST::Clause
{
public:
    void Visit(AST::Visitor &v) const override
    {
    }

    void AssertFacts(Database &db) const override
    {

    }

    std::shared_ptr<Relation> sink;

    std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override
    {
        return std::make_shared<NoneEvaluation>();
    }
};

class TableWriterClause : public AST::Clause
{
public:
    TableWriterClause(AST::Clause & lhs) : lhs(lhs)
    {
    }
    
    void Visit(AST::Visitor &v) const override { }
    void AssertFacts(Database &db) const override {}
        
    std::shared_ptr<Evaluation> Compile(Database &db, Compilation &c) override
    {
        return lhs.CompileLhs(db, c);
    }
    
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override
    {
        assert(!"Impossible");
        return std::make_shared<NoneEvaluation>();
    }

    void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override
    {
    }

    
private:
    AST::Clause & lhs;
};

void AST::Rule::Compile(Database &db)
{
    int locals_size=0;
    std::unordered_map<std::string, int> variables;

    std::vector<Entity> locals;

    Compilation compilation;
    
    TableWriterClause writer(*lhs);
    rhs->SetNext(writer);

    auto evaluation = rhs->Compile(db, compilation);
    
    evaluation = std::make_shared<RuleEvaluation>(std::move(compilation.row), evaluation);
    
    lhs->AddRule(db, evaluation);
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityIsPredicate::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityHasAttributes::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::NotImplementedClause::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::And::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityIs::Compile(Database &db, Compilation &compilation)
{
    /*
        Two cases:
        1) It's bound, either as a variable, or by a value
        2) It's unbound, by a named or an unnamed variable
    */
    bool bound = false;
    int slot = 0;

    if(const AST::Variable *v = entity->IsVariable())
    {
        if(auto *named = v->IsNamedVariable())
        {
            slot = compilation.AddVariable(named->name, bound);
        }
        else
        {
            slot = compilation.AddUnnamedVariable();
        }
    }
    else if(const Value *v = entity->IsValue())
    {
        slot = compilation.AddValue(v->MakeEntity(db));
        bound = true;
    }
    
    assert(next);
    auto eval = next->Compile(db, compilation);

    if( auto e = dynamic_cast<AST::UnaryPredicate*>(&*list))
    {
        auto relation = db.GetUnaryRelation(e->name);
        if(bound)
            eval = std::make_shared<EvaluateB>(relation, slot, eval);
        else
        {
            eval = std::make_shared<EvaluateF>(relation, slot, eval);
            bound = true;
        }
    }
    else if(auto l = dynamic_cast<AST::UnaryPredicateList*>(&*list))
    {
        for(auto i = l->list.rbegin(); i!=l->list.rend(); ++i)
        {
            auto relation = db.GetUnaryRelation((*i)->name);
            if(bound)
                eval = std::make_shared<EvaluateB>(relation, slot, eval);
            else
            {
                eval = std::make_shared<EvaluateF>(relation, slot, eval);
                bound = true;
            }

        }
    }
    else
    {
        assert(!"Invalid list type");
    }

    return eval;
}

Compilation::Compilation()
{
    //stack_size = 0;
}

Compilation::~Compilation()
{
}

int Compilation::AddVariable(const std::string &name, bool &alreadybound)
{
    auto i = variables.find(name);
    
    alreadybound = i != variables.end();

    if( alreadybound )
    {
        return i->second;
    }
    else
    {
        auto size = row.size();
        row.push_back(Entity());
        return variables[name] = size;
    }
}

int Compilation::AddUnnamedVariable()
{
    auto size = row.size();
    row.push_back(Entity());
    return size;
}

int Compilation::AddValue(const Entity &e)
{
    auto size = row.size();
    row.push_back(e);
    return size;
}

std::shared_ptr<Evaluation> AST::Or::Compile(Database &db, Compilation & compilation)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::Not::Compile(Database &db, Compilation & compilation)
{
    return std::make_shared<NoneEvaluation>();
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
                
void EvaluateF::Evaluate(Entity * row)
{
    class Visitor : public Relation::Visitor
    {
    public:
        Visitor(Entity * row, int slot, Evaluation & next) :
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
    } v(row, slot, *next);
    
    relation->Query(row+slot, v);
}

void EvaluateB::Evaluate(Entity * row)
{
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityIs::CompileLhs(Database &db, Compilation &c)
{
    int slot;
    if(const AST::Value *v = entity->IsValue())
    {
        slot = c.AddValue(v->MakeEntity(db));
    }
    else if(const AST::Variable *v = entity->IsVariable())
    {
        if(const AST::NamedVariable *nv = v->IsNamedVariable())
        {
            bool bound;
            slot = c.AddVariable(nv->name, bound);
            if(bound)
            {
                if(AST::UnaryPredicate * up = dynamic_cast<UnaryPredicate*>(&*list))
                {
                    return std::make_shared<WriterB>(db.GetUnaryRelation(up->name), slot);
                }
                else if(AST::UnaryPredicateList *l = dynamic_cast<UnaryPredicateList*>(&*list))
                {
                    std::shared_ptr<Evaluation> result;
                    for(auto & i : l->list)
                    {
                        auto e = std::make_shared<WriterB>(db.GetUnaryRelation(i->name), slot);
                        if(result)
                            result = std::make_shared<OrEvaluation>(result, e);
                        else
                            result = e;
                    }
                    return result;
                }
                else
                {
                    assert(!"Impossible");
                }
            }
            if(!bound)
            {
                db.UnboundError(nv->name);
            }
        }
        else
        {
            // Unbound variable
            db.UnboundError("_");
        }
    }
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityHasAttributes::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::NotImplementedClause::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityIsPredicate::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::And::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::Or::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::Not::CompileLhs(Database &db, Compilation &c)
{
    db.InvalidLhs();
    return std::make_shared<NoneEvaluation>();
}

void AST::DatalogPredicate::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // TODO
}

void AST::EntityIsPredicate::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // TODO
}

void AST::EntityHasAttributes::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // TODO
}

void AST::NotImplementedClause::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::And::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // ??
}

void AST::Or::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::Not::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::EntityIs::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    if(UnaryPredicate *u = dynamic_cast<UnaryPredicate*>(&*list))
    {
        db.GetUnaryRelation(u->name)->AddRule(rule);

    }
    else if(UnaryPredicateList *l = dynamic_cast<UnaryPredicateList*>(&*list))
    {
        for(auto &i : l->list)
            db.GetUnaryRelation(i->name)->AddRule(rule);
    }
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
