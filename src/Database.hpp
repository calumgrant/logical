
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "StringTable.hpp"
#include "Relation.hpp"
#include "Colours.hpp"

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

    int operator()(const std::pair<int, int> &p) const
    {
        return p.first * 97 + p.second;
    }

};

class Database
{
public:
    Database();
    ~Database();
    
    // File operations
    int ReadFile(const char * filename);
    void ReadBinary(const char * filename);
    void WriteBinary(const char * filename);

    // Create entities
    Entity CreateString(const std::string&s) { return Entity { EntityType::String, strings.GetId(s) }; }
    Entity CreateInt(int i) const { return Entity { EntityType::Integer, i}; }
    Entity CreateFloat(float f) const { return Entity { EntityType::Float, f}; }
    Entity CreateAt(const std::string &s) { return Entity { EntityType::AtString, atstrings.GetId(s) }; }
    Entity Create(bool b) const { return Entity { EntityType::Boolean, b}; }
    
    Entity AddStrings(int id1, int id2);
    
    int GetStringId(const std::string&s);
    int GetAtStringId(const std::string&s);
    int GetStringLiteral(const char * literal);

    void Add(const std::string & table, const Entity &entityId);
    void Add(const std::string & table, const Entity &entityId1, const Entity &entity);

    void SyntaxError(const SourceLocation&);

    // Variable "name" is not bound to a value
    void UnboundError(const std::string &name, int line, int column);

    void NotImplementedError(const SourceLocation&);
    
    std::shared_ptr<Relation> GetUnaryRelation(int nameId);
    std::shared_ptr<Relation> GetBinaryRelation(int nameId);
    std::shared_ptr<Relation> GetRelation(int nameId, int arity);
    std::shared_ptr<Relation> GetRelation(const CompoundName &cn);

    const std::string &GetString(int id) const;
    const std::string &GetAtString(int id) const;

    void Find(int unaryPredicateId);

    void Print(const Entity &e, std::ostream &os) const;
    
    // Same as Print, but put quotes around strings.
    void PrintQuoted(const Entity &e, std::ostream &os) const;

    // Logs an error for invalid left hand side clause
    void InvalidLhs();
    
    // Options for logging output
    // True if we want explanations
    void SetVerbose(bool value=true);
    bool Explain() const;

    void ReportUserError();
    bool UserErrorReported() const;
    
    static std::size_t GlobalCallCount();

    int NumberOfErrors() const;
    int NumberOfResults() const;
    
    void WarningEmptyRelation(Relation&);
    
    void RunQueries();
    
    void AddResult(const Entity * row, int arity, bool displayFirstColumn);
    
    // Gets the ANSI colour sequence for the given highlight, if appropriate for the platform.
    // Otherwise returns the empty string.
    const char * Highlight(Colours::TextHighlight highlight);

private:
    std::unordered_map< int, std::shared_ptr<Relation> > unaryRelations;
    std::unordered_map< int, std::shared_ptr<Relation> > binaryRelations;
    std::unordered_map< std::pair<int, int>, std::shared_ptr<Relation>, RelationHash> relations;

    StringTable strings, atstrings;
    
    bool verbose = false;
    int errorCount = 0;
    std::size_t resultCount = 0;
    
    std::unordered_map<CompoundName, std::shared_ptr<Relation>, CompoundName::Hash> tables;
    
    // Names, indexed on their first column
    std::unordered_multimap<int, CompoundName> names;

    std::shared_ptr<Relation> queryPredicate;
    
    void CreateProjection(const CompoundName &from, const CompoundName & to);
};
