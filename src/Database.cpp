#include "Database.hpp"

#include <iostream>

UnaryRelation& Database::GetUnaryRelation(const std::string & name)
{
    auto i = unaryRelations.find(name);
    if (i == unaryRelations.end())
    {
        auto p = std::make_unique<UnaryTable>();
        auto & result = *p;
        unaryRelations.insert(std::make_pair(name, std::move(p)));
        return result;
    }
    else
        return *i->second;
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

UnaryRelation::~UnaryRelation()
{
}

Database::Database()
{
    unaryRelations["print"] = std::make_unique<PrintRelation>(*this);
}

Database::~Database()
{
}

PrintRelation::PrintRelation(Database &db) : database(db)
{
}

void PrintRelation::Add(const Entity &e)
{
    switch(e.type)
    {
    case EntityType::Integer:
        std::cout << e.i << std::endl;
        break;
    case EntityType::Float:
        std::cout << e.f << std::endl;
        break;
    case EntityType::Boolean:
        std::cout << (e.i?"true":"false") << std::endl;
        break;
    case EntityType::String:
        std::cout << database.GetString(e.i) << std::endl;
        break;
    case EntityType::AtString:
        std::cout << "@" << database.GetAtString(e.i) << std::endl;
        break;
    }
}

int PrintRelation::size() const
{
    return 0;
}

const std::string &Database::GetString(int id) const
{
    return strings.GetString(id);
}

const std::string &Database::GetAtString(int id) const
{
    return atstrings.GetString(id);
}
