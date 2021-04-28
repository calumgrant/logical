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

BinaryRelation & Database::GetBinaryRelation(const std::string & name)
{
    auto i = binaryRelations.end();
    if (i==binaryRelations.end())
    {
        auto p = std::make_unique<BinaryTable>();
        auto & result = *p;
        binaryRelations.insert(std::make_pair(name, std::move(p)));
        return result;
    }
    else
        return *i->second;
}

void UnaryTable::Add(const Entity &e)
{
    //std::cout << "Added (" << (int)e.type << "," << e.i << ") to the table\n";
    values.insert(e);
}

void BinaryTable::Add(const Entity &e1, const Entity &e2)
{
    //std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
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

Relation::~Relation()
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

void PrintRelation::Add(const Entity &e)
{
    database.Print(e, std::cout);
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

int TableX::size() const
{
    return 0;
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

    GetUnaryRelation(unaryPredicateName).Visit(visitor);

    std::cout << "Found " << visitor.count << " rows\n";
}

void UnaryTable::Visit(Visitor &v) const
{
    for(auto &i : values)
    {
        v.OnRow(&i);
    }
}

void PrintRelation::Visit(Visitor&) const
{
    // Empty relation.
}

void BinaryTable::Visit(Visitor&v) const
{
    // todo
}

void TableX::Visit(Visitor&v) const
{
    // todo
}