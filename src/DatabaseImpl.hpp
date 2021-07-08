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
    
    StringId GetStringId(const char *s) override;
    StringId GetAtStringId(const char *s) override;
    StringId GetStringId(const string_type&s) override;
    StringId GetAtStringId(const string_type&s) override;
    const string_type &GetString(StringId id) const override;
    const string_type &GetAtString(StringId id) const override;
    
    Relation& GetRelation(const PredicateName&) override;
    
    void Find(const PredicateName & unaryPredicateName) override;
    void SetVerbosity(int v) override;
    void AddResult(const Entity * row, int arity, bool displayFirstColumn) override;
    int GetVerbosity() const override;
    
    void RunQueries();
    int NumberOfErrors() const override;
    int NumberOfResults() const;
    void ReportUserError() override;
    bool AnsiHighlightingEnabled() const override;
    
    void SetExpectedResults(int count) override;
    
    void CheckErrors();

    Optimizer & GetOptimizer() const override;
    
    void SetAnsiColours(bool);
    
    Relation &GetQueryRelation() const override;
    
    persist::shared_memory & Storage() override;
    
    Entity NewEntity() override;

    void LoadModule(const char*) override;

    void AddRelation(const std::shared_ptr<Relation> & rel);
    Relation & GetExtern(const PredicateName & pn) override;

private:
    Optimizer & optimizer;
    
    persist::map_file datafile;
    persist::map_data<DataStore> datastore;
    
    int verbosity = 1;
    int errorCount = 0;
    std::size_t resultCount = 0;
    
    void CreateProjection(const PredicateName &from, const PredicateName & to);

    // -1 means there is no expected value.
    int expectedResults = -1;
    
    void MakeReachesRelation(Relation & rel);
    void MakeProjections(Relation & rel);
};
