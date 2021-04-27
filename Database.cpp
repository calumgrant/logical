#include "Database.hpp"
#include "AST.hpp"

// Need to create a pure parser and remove this global
Database database;

void ProcessFact(AST::Term * f)
{
    std::unique_ptr<AST::Term> fact(f);
    if(f)
    {
        f->AssertFacts(database);
    }
    else
    {
        std::cout << "Error: null fact\n";
    }
}

void ProcessRule(AST::Term * lhs, AST::Term * rhs)
{
    std::unique_ptr<AST::Term> l(lhs), r(rhs);
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
    std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
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
