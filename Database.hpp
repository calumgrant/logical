
#include <string>
#include <unordered_map>

typedef int EntityId;

// Something that's stored in the database
struct Entity
{
    union
    {
        int i;
        float f;
    };
    
};

class UnaryTable
{
public:
    void Add(const Entity &e);
};

class BinaryTable
{
public:
    void Add(const Entity &e1, const Entity &e2);
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
private:
    std::unordered_map<std::string, UnaryTable> unaryRelations;
    std::unordered_map<std::string, BinaryTable> binaryRelations;
};