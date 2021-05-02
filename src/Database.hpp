
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "StringTable.hpp"
#include "Relation.hpp"

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

    std::shared_ptr<Relation> GetUnaryRelation(const std::string &name);
    std::shared_ptr<Relation> GetBinaryRelation(const std::string &name);
    std::shared_ptr<Relation> GetRelation(const std::string &name, int arity);

    const std::string &GetString(int id) const;
    const std::string &GetAtString(int id) const;

    void Find(const std::string & unaryPredicateName);

    void Print(const Entity &e, std::ostream &os) const;
    
    
    // Logs an error for invalid left hand side clause
    void InvalidLhs();
    
    // Options for logging output
    // True if we want explanations
    void SetVerbose(bool value=true);
    bool Explain() const;
private:
    std::unordered_map< std::string, std::shared_ptr<Relation> > unaryRelations;
    std::unordered_map< std::string, std::shared_ptr<Relation> > binaryRelations;
    std::unordered_map< std::pair<std::string, int>, std::shared_ptr<Relation>, RelationHash> relations;

    StringTable strings, atstrings;
    
    bool verbose;
};
