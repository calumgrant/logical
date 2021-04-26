
#include <string>
#include <unordered_map>
#include <Entity.hpp>
#include <StringTable.hpp>

class Relation
{
public:
    virtual void Add(const Entity &e)=0;
};

class UnaryTable : public Relation
{
public:
    void Add(const Entity &e);
};

class BinaryTable : public Relation
{
public:
    void Add(const Entity &e);
    void Add(const Entity &e1, const Entity &e2);
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
    void UnboundError(const std::string &name, const SourceLocation&);

    void NotImplementedError(const SourceLocation&);

    Relation &GetRelation(const std::string &name, int arity);
private:
    std::unordered_map<std::string, UnaryTable> unaryRelations;
    std::unordered_map<std::string, BinaryTable> binaryRelations;

    // String table
    StringTable strings, atstrings;
};
