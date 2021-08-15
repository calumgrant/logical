#include "persist.h"
#include "Analysis.hpp"

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

class DatabaseImpl : public Database
{
public:
    DatabaseImpl(Optimizer & optimizer, const char * datafile, int limitMB);
    ~DatabaseImpl();

    int ReadFile(const char * filename) override;

    StringId GetStringId(const char *s) override;
    StringId GetAtStringId(const char *s) override;
    const char* GetString(StringId id) const override;
    const char* GetAtString(StringId id) const override;
    
    Relation& GetRelation(const PredicateName&) override;
    
    void Find(const PredicateName & unaryPredicateName) override;
    void SetVerbosity(int v) override;
    void AddResult(const Entity * row, int arity, bool displayFirstColumn) override;
    void AddResult() override;
    int GetVerbosity() const override;
    
    int NumberOfErrors() const override;
    int NumberOfResults() const;
    void ReportUserError() override;
    bool AnsiHighlightingEnabled() const override;
    
    void SetExpectedResults(int count) override;
    void SetExpectedErrors(int count) override;

    void CheckErrors();

    Optimizer & GetOptimizer() const override;
    
    void SetAnsiColours(bool);
        
    AllocatorData & Storage() override;
    persist::shared_memory & SharedMemory() override;
    
    Entity NewEntity() override;

    void LoadModule(const char*) override;

    void AddRelation(const std::shared_ptr<Relation> & rel);
    Relation & GetExtern(const PredicateName & pn) override;
    void Addvarargs(RelationId name, Logical::Extern fn, void * data) override;
    void SetMemoryLimit(std::size_t) override;
    bool IsExtern(StringId name) const override;

private:
    Optimizer & optimizer;
    
    persist::map_file datafile;
    MemoryCounter memoryCounter;
    persist::map_data<DataStore> datastore;
    
    int verbosity = 1;
    int errorCount = 0;
    std::size_t resultCount = 0;
    
    void CreateProjection(const PredicateName &from, const PredicateName & to, const SourceLocation & loc);

    // -1 means there is no expected value.
    int expectedResults = -1;
    int expectedErrors = 0;
    
    void MakeReachesRelation(Relation & rel, const SourceLocation & loc);
    void MakeProjections(Relation & rel, const SourceLocation & loc);
};
