#include "Compiler.hpp"
#include "Database.hpp"
#include "Evaluation.hpp"

#include <iostream>

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

void AST::Rule::Compile(Database &db)
{
    int locals_size=0;
    std::unordered_map<std::string, int> variables;

    std::vector<Entity> locals;
    VariableNumberer numberer;

    Visit(numberer);

    std::cout << "Rule has " << numberer.variables.size() << " variables\n";

    Compilation compilation;

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



    return std::make_shared<NoneEvaluation>();
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
