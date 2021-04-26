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
    else
    {
        std::cout << "Error: null fact\n";
    }
}

void ProcessRule(AST::Clause * lhs, AST::Clause * rhs)
{
    std::unique_ptr<AST::Clause> l(lhs), r(rhs);
}

UnaryTable & Database::GetUnaryRelation(const std::string & name)
{
    return unaryRelations[name];
}

BinaryTable & Database::GetBinaryRelation(const std::string & name)
{
    return binaryRelations[name];
}

void UnaryTable::Add(const Entity &e)
{
    std::cout << "Added (" << (int)e.type << "," << e.i << ") to the table\n";
    values.insert(e);
}

void BinaryTable::Add(const Entity &e1, const Entity &e2)
{
    values.insert(std::make_pair(e1,e2));
}

int UnaryTable::size() const
{
    return values.size();
}

int BinaryTable::size() const
{
    return values.size();
}

void Database::UnboundError(const std::string &name)
{
    std::cerr << "Error: " << name << " is unbound.\n";
}
