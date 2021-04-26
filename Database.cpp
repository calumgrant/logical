#include "Database.hpp"
#include "Clause.hpp"

// Need to create a pure parser and remove this global
Database database;

void ProcessFact(AST::Clause * f)
{
    std::unique_ptr<AST::Clause> fact(f);
    if(f)
    {
        f->AssertFacts(database);
    }
}

void ProcessRule(AST::Clause * lhs, AST::Clause * rhs)
{
    std::unique_ptr<AST::Clause> l(lhs), r(rhs);
}

Relation & Database::GetRelation(const std::string & name, int arity)
{
    if(arity==1) return unaryRelations[name];
    if(arity==2) return binaryRelations[name];

    throw std::logic_error("Invalid arity");
}

void UnaryTable::Add(const Entity &e)
{

}

void BinaryTable::Add(const Entity &e)
{
    
}
