#include "Database.hpp"

#include <iostream>

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
