
#include <string>
#include <unordered_map>

typedef int EntityId;

enum class EntityType
{
    Integer,
    String,
    Float,
    AtEntity,
    Boolean
};

// Something that's stored in the database
struct Entity
{
    union
    {
        int i;
        float f;
    };
    
};

class Relation
{

};

class UnaryTable : public Relation
{
public:
    void Add(const Entity &e);
};

class BinaryTable : public Relation
{
public:
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
    Entity CreateString(const std::string&);
    Entity CreateInt(int d);
    Entity CreateFloat(double d);
    Entity CreateAt(const std::string &);
    Entity Create(bool b);

    void Add(const std::string & table, const Entity &entityId);
    void Add(const std::string & table, const Entity &entityId1, const Entity &entity);

    void SyntaxError(const SourceLocation&);

    // Variable "name" is not bound to a value
    void UnboundError(const std::string &name, const SourceLocation&);

    void NotImplementedError(const SourceLocation&);
private:
    std::unordered_map<std::string, UnaryTable> unaryRelations;
    std::unordered_map<std::string, BinaryTable> binaryRelations;
};
