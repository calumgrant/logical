#include "Database.hpp"

#include <iostream>

std::shared_ptr<Relation> Database::GetUnaryRelation(const std::string & name)
{
    auto i = unaryRelations.find(name);
    if (i == unaryRelations.end())
    {
        auto p = std::make_shared<UnaryTable>(name);
        unaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
        return i->second;
}

std::shared_ptr<Relation> Database::GetBinaryRelation(const std::string & name)
{
    auto i = binaryRelations.end();
    if (i==binaryRelations.end())
    {
        auto p = std::make_shared<BinaryTable>(name);
        binaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
        return i->second;
}

void Database::UnboundError(const std::string &name)
{
    std::cerr << "Error: " << name << " is unbound.\n";
}

Relation::~Relation()
{
}

Database::Database() : verbose(false), userError(false)
{
    unaryRelations["print"] = std::make_shared<PrintRelation>(std::cout, *this);
    unaryRelations["error"] = std::make_shared<ErrorRelation>(*this);
}

Database::~Database()
{
}

void Database::Print(const Entity &e, std::ostream &os) const
{
    switch(e.type)
    {
    case EntityType::None:
            os << "None";
            break;
    case EntityType::Integer:
        os << e.i;
        break;
    case EntityType::Float:
        os << e.f;
        break;
    case EntityType::Boolean:
        os << (e.i?"true":"false");
        break;
    case EntityType::String:
        os << GetString(e.i);
        break;
    case EntityType::AtString:
        os << "@" << GetAtString(e.i);
        break;
    }
}

void Database::PrintQuoted(const Entity &e, std::ostream &os) const
{
    if(e.type == EntityType::String)
    {
        os << '\"';
        Print(e, os);
        os << '\"';
    }
    else
    {
        Print(e, os);
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

std::shared_ptr<Relation> Database::GetRelation(const std::string &name, int arity)
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
        auto r = std::make_shared<TableX>(name);
        relations.insert(std::make_pair(index, r));
        return r;
    }
    else
        return i->second;
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
            std::cout << std::endl;
            ++count;
        }
    };
    Tmp visitor(*this);

    Entity row;
    GetUnaryRelation(unaryPredicateName)->Query(&row, 0, visitor);

    std::cout << "Found " << visitor.count << " rows\n";
}

void Database::InvalidLhs()
{
    std::cerr << "Invalid left hand side of a rule.\n";
}

void Database::SetVerbose(bool v)
{
    verbose = v;
}

bool Database::Explain() const
{
    return verbose;
}

bool Database::UserErrorReported() const
{
    return userError;
}

void Database::ReportUserError()
{
    userError = true;
}
