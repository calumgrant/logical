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
    
    void Visit(AST::Visitor &v) const { }
    void AssertFacts(Database &db) const {}
        
    std::shared_ptr<Evaluation> Compile(Database &db, Compilation &c)
    {
        return lhs.CompileLhs(db, c);
    }
    
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation)
    {
        assert(!"Impossible");
        return std::make_shared<NoneEvaluation>();
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
    stack_size = 0;
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
        return variables[name] = stack_size++;
    }
}

int Compilation::AddUnnamedVariable()
{
    return stack_size++;
}

int Compilation::AddValue(const Entity &e)
{
    // TODO: Store the value
    return stack_size++;
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
                    // Not implemented...
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
