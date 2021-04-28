#include "Database.hpp"

#include <iostream>

Relation& Database::GetUnaryRelation(const std::string & name)
{
    auto i = unaryRelations.find(name);
    if (i == unaryRelations.end())
    {
        auto p = std::make_shared<UnaryTable>();
        auto & result = *p;
        unaryRelations.insert(std::make_pair(name, std::move(p)));
        return result;
    }
    else
        return *i->second;
}

Relation & Database::GetBinaryRelation(const std::string & name)
{
    auto i = binaryRelations.end();
    if (i==binaryRelations.end())
    {
        auto p = std::make_shared<BinaryTable>();
        auto & result = *p;
        binaryRelations.insert(std::make_pair(name, std::move(p)));
        return result;
    }
    else
        return *i->second;
}

void Database::UnboundError(const std::string &name)
{
    std::cerr << "Error: " << name << " is unbound.\n";
}

Relation::~Relation()
{
}

Database::Database()
{
    unaryRelations["print"] = std::make_shared<PrintRelation>(*this);
}

Database::~Database()
{
}

void Database::Print(const Entity &e, std::ostream &os) const
{
    switch(e.type)
    {
    case EntityType::Integer:
        os << e.i << std::endl;
        break;
    case EntityType::Float:
        os << e.f << std::endl;
        break;
    case EntityType::Boolean:
        os << (e.i?"true":"false") << std::endl;
        break;
    case EntityType::String:
        os << GetString(e.i) << std::endl;
        break;
    case EntityType::AtString:
        os << "@" << GetAtString(e.i) << std::endl;
        break;
    }
}


const std::string &Database::GetString(int id) const
{
    return strings.GetString(id);
}

const std::string &Database::GetAtString(int id) const
{
    return atstrings.GetString(id);
}

Relation &Database::GetRelation(const std::string &name, int arity)
{
    switch(arity)
    {
    case 1: return GetUnaryRelation(name);
    case 2: return GetBinaryRelation(name);
    }

    auto index = std::make_pair(name, arity);

    auto i = relations.find(index);

    if (i == relations.end())
    {
        auto r = std::make_unique<TableX>();
        auto & result = *r;
        relations.insert(std::make_pair(index, std::move(r)));
        return result;
    }
    else
        return *i->second;
}

void Database::Find(const std::string & unaryPredicateName)
{
    class Tmp : public Relation::Visitor
    {
    public:
        Database &db;
        int count;
        Tmp(Database &db) : db(db), count() { }

        void OnRow(const Entity *e) override
        {
            std::cout << "\t";
            db.Print(*e, std::cout);
            ++count;
        }
    };
    Tmp visitor(*this);

    Entity row;
    GetUnaryRelation(unaryPredicateName).Query(&row, visitor);

    std::cout << "Found " << visitor.count << " rows\n";
}
