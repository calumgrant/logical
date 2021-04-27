
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Entity.hpp"
#include "StringTable.hpp"
#include "AST.hpp"


class Relation
{
public:
};

class UnaryTable : public Relation
{
public:
    void Add(const Entity &e);
    std::unordered_set<Entity, Entity::Hash> values;
    int size() const;
};

struct PairHash
{
    int operator()(const std::pair<Entity, Entity> &value) const
    {
        Entity::Hash h;
        return h(value.first) * 13 + h(value.second);
    }
};

class BinaryTable : public Relation
{
public:
    void Add(const Entity &e1, const Entity &e2);
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    int size() const;
};

class SourceLocation
{
    int lineNumber;
};

// ?? 
class Error
{

};

class Database
{
public:

    // Create entities
    Entity CreateString(const std::string&s) { return Entity { EntityType::String, strings.GetId(s) }; }
    Entity CreateInt(int i) { return Entity { EntityType::Integer, i}; }
    Entity CreateFloat(float f) { return Entity { EntityType::Float, f}; }
    Entity CreateAt(const std::string &s)  { return Entity { EntityType::AtString, atstrings.GetId(s) }; }
    Entity Create(bool b) { return Entity { EntityType::Boolean, b}; }

    void Add(const std::string & table, const Entity &entityId);
    void Add(const std::string & table, const Entity &entityId1, const Entity &entity);

    void SyntaxError(const SourceLocation&);

    // Variable "name" is not bound to a value
    void UnboundError(const std::string &name);

    void NotImplementedError(const SourceLocation&);

    UnaryTable &GetUnaryRelation(const std::string &name);
    BinaryTable &GetBinaryRelation(const std::string &name);

    // What to do
    void ProcessFact(AST::Term * statement);
    void ProcessRule(AST::Term * lhs, AST::Term * rhs);

private:
    std::unordered_map<std::string, UnaryTable> unaryRelations;
    std::unordered_map<std::string, BinaryTable> binaryRelations;

    // String table
    StringTable strings, atstrings;
};
