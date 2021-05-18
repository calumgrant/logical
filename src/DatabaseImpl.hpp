#include <unordered_map>
#include <unordered_set>

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
    DatabaseImpl();
    ~DatabaseImpl();
    
    int GetStringId(const std::string&s) override;
    int GetAtStringId(const std::string&s) override;
    const std::string &GetString(int id) const override;
    const std::string &GetAtString(int id) const override;
    
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
    
    // -1 means there is no expected value.
    int expectedResults = -1;
};
