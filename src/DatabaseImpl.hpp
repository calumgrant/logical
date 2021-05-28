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
    DatabaseImpl(const char * name, int limitMB);
    ~DatabaseImpl();
    
    StringId GetStringId(const char *s) override;
    StringId GetAtStringId(const char *s) override;
    StringId GetStringId(const string_type&s) override;
    StringId GetAtStringId(const string_type&s) override;
    const string_type &GetString(StringId id) const override;
    const string_type &GetAtString(StringId id) const override;
    
    std::shared_ptr<Relation> GetUnaryRelation(int name) override;
    std::shared_ptr<Relation> GetBinaryRelation(int name) override;
    std::shared_ptr<Relation> GetRelation(int name, int arity) override;
    std::shared_ptr<Relation> GetRelation(const CompoundName &cn) override;

    void Find(int unaryPredicateName) override;
    void SetVerbose(bool v) override;
    bool Explain() const override;
    void AddResult(const Entity * row, int arity, bool displayFirstColumn) override;

    void RunQueries();
    int NumberOfErrors() const override;
    int NumberOfResults() const;
    void ReportUserError() override;
    bool AnsiHighlightingEnabled() const override;
    
    void SetExpectedResults(int count) override;
    
    void CheckErrors();
    
    void SetOptimizationLevel(int level);
    
    OptimizationOptions options;
    
    const OptimizationOptions & Options() const override;
    
    void SetAnsiColours(bool);
    
    Relation &GetQueryRelation() const override;

private:
    persist::map_file datafile;
    persist::map_data<DataStore> datastore;
    
    bool verbose = false;
    int errorCount = 0;
    std::size_t resultCount = 0;
    
    void CreateProjection(const CompoundName &from, const CompoundName & to);
    
    // -1 means there is no expected value.
    int expectedResults = -1;
};
