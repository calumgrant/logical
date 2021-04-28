
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Entity.hpp"
#include "StringTable.hpp"

class Database;

class Relation
{
public:
    class Visitor
    {
    public:
        virtual void OnRow(const Entity*)=0;
    };

    virtual void Visit(Visitor&v) const =0;

    virtual ~Relation();
    virtual int size() const =0;
};

class UnaryRelation : public Relation
{
public:
    virtual void Add(const Entity &e)=0;
};

class UnaryTable : public UnaryRelation
{
public:
    void Add(const Entity &e) override;
    std::unordered_set<Entity, Entity::Hash> values;
    int size() const override;
    void Visit(Visitor&v) const override;
};

class PrintRelation : public UnaryRelation
{
public:
    PrintRelation(Database&db);
    void Add(const Entity &e) override;
    int size() const override;
    void Visit(Visitor&v) const override;
private:
    Database &database;
};

struct PairHash
{
    int operator()(const std::pair<Entity, Entity> &value) const
    {
        Entity::Hash h;
        return h(value.first) * 13 + h(value.second);
    }
};

class BinaryRelation : public Relation
{
public:
    virtual void Add(const Entity &e1, const Entity &e2) =0;
};

template<int N>
struct Row
{
    Entity data[N];
};

class BinaryTable : public BinaryRelation
{
public:
    void Add(const Entity &e1, const Entity &e2) override;
    std::unordered_set<std::pair<Entity, Entity>, PairHash> values;
    int size() const override;
    void Visit(Visitor&v) const override;
};

class TableX : public Relation
{
public:
    int size() const override;
    void Visit(Visitor&v) const override;
    int arity;
    std::vector<Entity> data;
};

class SourceLocation
{
    int lineNumber;
};

// ?? 
class Error
{

};

struct RelationHash
{
    int operator()(const std::pair<std::string, int> &p) const
    {
        return std::hash<std::string>()(p.first)+p.second;
    }
};

class Database
{
public:
    Database();
    ~Database();

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

    UnaryRelation &GetUnaryRelation(const std::string &name);
    BinaryRelation &GetBinaryRelation(const std::string &name);
    Relation &GetRelation(const std::string &name, int arity);

    const std::string &GetString(int id) const;
    const std::string &GetAtString(int id) const;

    void Find(const std::string & unaryPredicateName);

    void Print(const Entity &e, std::ostream &os) const;

private:
    std::unordered_map<std::string, std::unique_ptr<UnaryRelation>> unaryRelations;
    std::unordered_map<std::string, std::unique_ptr<BinaryRelation>> binaryRelations;
    std::unordered_map< std::pair<std::string, int>, std::unique_ptr<Relation>, RelationHash> relations;

    StringTable strings, atstrings;
};
