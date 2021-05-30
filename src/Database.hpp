
#include "Fwd.hpp"
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


class Database
{
public:
    // File operations
    int ReadFile(const char * filename);
    void ReadBinary(const char * filename);
    void WriteBinary(const char * filename);

    // Create entities
    Entity CreateString(const string_type&s) { return Entity { EntityType::String, GetStringId(s) }; }
    Entity CreateString(const char *s) { return Entity { EntityType::String, GetStringId(s) }; }
    Entity CreateInt(int i) const { return Entity { EntityType::Integer, i}; }
    Entity CreateFloat(float f) const { return Entity { EntityType::Float, f}; }
    Entity CreateAt(const string_type &s) { return Entity { EntityType::AtString, GetAtStringId(s) }; }
    Entity Create(bool b) const { return Entity { EntityType::Boolean, b}; }
    
    Entity AddStrings(StringId id1, StringId id2);
    
    virtual StringId GetStringId(const string_type&s) =0;
    virtual StringId GetAtStringId(const string_type&s) =0;
    virtual StringId GetStringId(const char*s) =0;
    virtual StringId GetAtStringId(const char*s) =0;
    int GetStringLiteral(const char * literal);
    virtual const string_type &GetString(StringId id) const =0;
    virtual const string_type &GetAtString(StringId id) const =0;

    void Add(const std::string & table, const Entity &entityId);
    void Add(const std::string & table, const Entity &entityId1, const Entity &entity);

    void SyntaxError(const SourceLocation&);

    // Variable "name" is not bound to a value
    void UnboundError(const char *name, int line, int column);

    void NotImplementedError(const SourceLocation&);
    
    virtual std::shared_ptr<Relation> GetUnaryRelation(RelationId nameId) =0;
    virtual std::shared_ptr<Relation> GetBinaryRelation(RelationId nameId) =0;
    virtual std::shared_ptr<Relation> GetRelation(RelationId nameId, Arity arity) =0;
    virtual std::shared_ptr<Relation> GetRelation(const CompoundName &cn) =0;
    virtual std::shared_ptr<Relation> GetReachesRelation(RelationId nameId) =0;

    virtual void Find(int unaryPredicateId) =0;

    void Print(const Entity &e, std::ostream &os) const;
    
    // Same as Print, but put quotes around strings and escapes control characters.
    void PrintQuoted(const Entity &e, std::ostream &os) const;

    // Logs an error for invalid left hand side clause
    void InvalidLhs();
    
    // Options for logging output
    // True if we want explanations
    virtual void SetVerbose(bool value=true) =0;
    virtual bool Explain() const =0;

    virtual void ReportUserError() =0;
    bool UserErrorReported() const;
    
    static std::size_t GlobalCallCountLimit();
    static std::size_t GlobalCallCount();

    void WarningEmptyRelation(Relation&);
    
    virtual void AddResult(const Entity * row, int arity, bool displayFirstColumn) =0;
    
    // Gets the ANSI colour sequence for the given highlight, if appropriate for the platform.
    // Otherwise returns the empty string.
    const char * Highlight(Colours::TextHighlight highlight);
    
    virtual bool AnsiHighlightingEnabled() const =0;
    
    virtual int NumberOfErrors() const =0;
    
    virtual void SetExpectedResults(int count) =0;
    void Error(const char*message);
    void SetEvaluationLimit(std::size_t count);
    
    bool EvaluationLimitExceeded();
    
    void ParityError(Relation &relation);
    
    virtual Optimizer & GetOptimizer() const =0;

    virtual Relation & GetQueryRelation() const = 0;
    
    virtual persist::shared_memory & Storage() =0;
};
