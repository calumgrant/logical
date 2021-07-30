
#include "Fwd.hpp"
#include "StringTable.hpp"
#include "Relation.hpp"
#include "Colours.hpp"

class Database
{
public:
    // File operations
    int ReadFile(const char * filename);
    void ReadBinary(const char * filename);
    void WriteBinary(const char * filename);

    // Create entities
    Entity CreateString(const char *s) { return Entity { EntityType::String, GetStringId(s) }; }
    Entity CreateInt(int i) const { return Entity { EntityType::Integer, i}; }
    Entity CreateFloat(float f) const { return Entity { EntityType::Float, f}; }
    Entity CreateAt(const char * s) { return Entity { EntityType::AtString, GetAtStringId(s) }; }
    Entity Create(bool b) const { return Entity { EntityType::Boolean, b}; }
    virtual Entity NewEntity() =0;
    
    Entity AddStrings(StringId id1, StringId id2);
    
    virtual StringId GetStringId(const char*s) =0;
    virtual StringId GetAtStringId(const char*s) =0;
    int GetStringLiteral(const char * literal);
    virtual const char * GetString(StringId id) const =0;
    virtual const char * GetAtString(StringId id) const =0;

    void Add(const std::string & table, const Entity &entityId);
    void Add(const std::string & table, const Entity &entityId1, const Entity &entity);

    void SyntaxError(const SourceLocation&);

    // Variable "name" is not bound to a value
    void UnboundError(const char *name, const SourceLocation & loc);

    void NotImplementedError(const SourceLocation&);
    
    virtual Relation& GetRelation(const PredicateName &pn) =0;

    virtual void Find(const PredicateName &pn) =0;

    void Print(const Entity &e, std::ostream &os) const;
    
    // Same as Print, but put quotes around strings and escapes control characters.
    void PrintQuoted(const Entity &e, std::ostream &os) const;

    // Logs an error for invalid left hand side clause
    void InvalidLhs();
    
    // Options for logging output
    // True if we want explanations
    virtual void SetVerbosity(int value) =0;
    bool Explain() const;
    virtual int GetVerbosity() const =0;
    bool LogRows() const;

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

    virtual AllocatorData & Storage() =0;
    virtual persist::shared_memory & SharedMemory() =0;

    virtual void LoadModule(const char*) =0;
    virtual Relation & GetExtern(const PredicateName & cn) =0;
    virtual void Addvarargs(RelationId name, Logical::Extern fn, void * data) =0;
    virtual void SetMemoryLimit(std::size_t) =0;
};

struct ParseData
{
    int filenameId;
    Database & db;
};
