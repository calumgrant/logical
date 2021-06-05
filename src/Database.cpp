#include "Database.hpp"
#include "Evaluation.hpp"
#include "tokens.tab.h"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "EvaluationImpl.hpp"
#include "DatabaseImpl.hpp"
#include "TableImpl.hpp"
#include "Helpers.hpp"

#include <unordered_map>
#include <unordered_set>
#include <iostream>


class DataStore
{
public:
    DataStore(persist::shared_memory & memory);
    
    StringTable strings, atstrings;
    
    unordered_map_helper<RelationId, std::shared_ptr<Relation>>::map_type unaryRelations, binaryRelations, reachesRelations;
    unordered_map_helper<std::pair<RelationId, Arity>, std::shared_ptr<Relation>, RelationHash>::map_type relations;
    unordered_map_helper<CompoundName, std::shared_ptr<Relation>, CompoundName::Hash>::map_type tables;
    
    // Names, indexed on their first column
    unordered_map_helper<StringId, CompoundName>::multimap_type names;

    std::shared_ptr<Relation> queryPredicate;
    
    std::atomic<std::int64_t> entityCounter = 0;
    
    bool initialized = false;
};

void yyrestart (FILE *input_file ,yyscan_t yyscanner );
int yylex_init (yyscan_t* scanner);
int yylex_init_extra (Database *, yyscan_t* scanner);
int yylex_destroy (yyscan_t yyscanner );
void yyset_in  (FILE * in_str ,yyscan_t yyscanner );

std::shared_ptr<Relation> DatabaseImpl::GetUnaryRelation(int name)
{
    auto i = datastore->unaryRelations.find(name);
    if (i == datastore->unaryRelations.end())
    {
        auto p = allocate_shared<Predicate>(datafile, *this, name, 1, false, BindingType::Unbound);

        datastore->unaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
    {
        return i->second;
    }
}

std::shared_ptr<Relation> DatabaseImpl::GetBinaryRelation(int name)
{
    auto i = datastore->binaryRelations.find(name);
    if (i==datastore->binaryRelations.end())
    {
        std::vector<int> cn(1);
        cn[0] = name;
        
        auto p = GetRelation(cn);
        datastore->binaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
        return i->second;
}

void Database::UnboundError(const char *name, int line, int column)
{
    std::cerr << "Error at (" << line << ":" << column << "): " << name << " is unbound.\n";
}

DatabaseImpl::DatabaseImpl(Optimizer & optimizer, const char * name, int limitMB) :
    datafile(name, 2, 2, 1, 16384, limitMB * 1000000ll, name ? 0 : persist::temp_heap),
    datastore(datafile.data(), datafile.data()),
    verbose(false),
    optimizer(optimizer)
{
    int queryId = GetStringId("query");
    
    if(!datastore->initialized)
    {
        datastore->queryPredicate = GetUnaryRelation(queryId);
        
        int print = GetStringId("print");
        AddRelation(allocate_shared<PrintRelation>(datafile, *this, print));
        AddRelation(allocate_shared<ErrorRelation>(datafile, *this));
        
        RelationId expected_results = GetStringId("expected-results");
        AddRelation(allocate_shared<ExpectedResults>(datafile, *this, expected_results));
        
        RelationId evaluation_step_limit = GetStringId("evaluation-step-limit");
        AddRelation(allocate_shared<EvaluationStepLimit>(datafile, *this, evaluation_step_limit));
        
        AddRelation(allocate_shared<Strlen>(datafile, *this));
        AddRelation(allocate_shared<Lowercase>(datafile, *this));
        AddRelation(allocate_shared<Uppercase>(datafile, *this));

        datastore->initialized = true;
    }
}

DatabaseImpl::~DatabaseImpl()
{
}

void Database::Print(const Entity &e, std::ostream &os) const
{
    switch(e.Type())
    {
    case EntityType::None:
            os << "None";
            break;
    case EntityType::Integer:
        os << (std::int64_t)e;
        break;
    case EntityType::Float:
        os << (double)e;
        break;
    case EntityType::Boolean:
        os << ((std::int64_t)e?"true":"false");
        break;
    case EntityType::String:
        os << GetString((std::int64_t)e);
        break;
    case EntityType::AtString:
        os << "@" << GetAtString((std::int64_t)e);
        break;
    case EntityType::Char:
    case EntityType::Byte:
        os << (std::int64_t)e;
        break;
    case EntityType::NewType:
        // I don't think it makes sense to output the actual identifier here.
        os << "ν"; //  << (std::int64_t)e;
        break;
    }
}

void Database::PrintQuoted(const Entity &e, std::ostream &os) const
{
    os << Colours::Value;

    if(e.IsString())
    {
        os << '\"';
        // TODO: Escape the characters
        Print(e, os);
        os << '\"';
    }
    else
    {
        Print(e, os);
    }
    os << Colours::Normal;
}

const string_type &DatabaseImpl::GetString(int id) const
{
    return datastore->strings.GetString(id);
}

const string_type &DatabaseImpl::GetAtString(int id) const
{
    return datastore->atstrings.GetString(id);
}

std::shared_ptr<Relation> DatabaseImpl::GetRelation(int name, int arity)
{
    switch(arity)
    {
    case 1: return GetUnaryRelation(name);
    case 2: return GetBinaryRelation(name);
    }

    auto index = std::make_pair(name, arity);

    auto i = datastore->relations.find(index);

    if (i == datastore->relations.end())
    {
        std::vector<int> p = { name };
        CompoundName cn(p);
        auto r = allocate_shared<Predicate>(datafile, *this, cn, arity, BindingType::Unbound);
        datastore->relations.insert(std::make_pair(index, r));
        return r;
    }
    else
        return i->second;
}

std::shared_ptr<Relation> DatabaseImpl::GetReachesRelation(RelationId nameId)
{
    auto i = datastore->reachesRelations.find(nameId);
    if(i==datastore->reachesRelations.end())
    {
        auto r = GetBinaryRelation(nameId);
        auto rel = allocate_shared<Predicate>(datafile, *this, nameId, 2, true, BindingType::Unbound);
        // Create the rules (TODO)

        {
            // Add the rule rel(_0,_1) :- r(_0, _1)
            std::vector<int> writeArgs = { 0, 1 };
            auto write = std::make_shared<Writer>(rel, writeArgs);
            auto reader = std::make_shared<Join>(r, std::vector<int>{-1,-1}, std::move(writeArgs), write);
            auto baseRule = std::make_shared<RuleEvaluation>(2, reader);
            rel->AddRule(baseRule);
        }
        
        {
            // Add the rule rel(_0, _1) :- rel(_0, _2), r(_2, _1).
            std::vector<int> writeArgs = { 0, 1 };
            auto write = std::make_shared<Writer>(rel, writeArgs);
            auto join2 = std::make_shared<Join>(r, std::vector<int> {2, -1}, std::vector<int> { -1, 1 }, write);
            auto join1 = std::make_shared<Join>(rel, std::vector<int> {-1,-1}, std::vector<int> {0, 2}, join2);
            auto recursiveRule = std::make_shared<RuleEvaluation>(3, join1);
            rel->AddRule(recursiveRule);
        }
        
        datastore->reachesRelations.insert(std::make_pair(nameId, rel));
        return rel;
    }
    else
        return i->second;
}

void DatabaseImpl::Find(int unaryPredicateName)
{
    class Tmp : public Receiver
    {
    public:
        Database &db;
        int count;
        Tmp(Database &db) : db(db), count() { }

        void OnRow(Entity *e) override
        {
            db.AddResult(e, 1, true);
            ++count;
        }
    };
    Tmp visitor(*this);

    Entity row;
    GetUnaryRelation(unaryPredicateName)->Query(&row, 0, visitor);

    std::cout << "Found " << visitor.count << " results\n";
}

void Database::InvalidLhs()
{
    std::cerr << "Invalid left hand side of a rule.\n";
}

void DatabaseImpl::SetVerbose(bool v)
{
    verbose = v;
}

bool DatabaseImpl::Explain() const
{
    return verbose;
}

int Database::GetStringLiteral(const char *literal)
{
    int len = strlen(literal)-1;
    std::string value;
    value.reserve(len);
    for(int i=1; i<len; ++i)
    {
        if(literal[i]=='\\')
        {
            ++i;
            switch(literal[i])
            {
            case 'r':
                value.push_back('\r');
                break;
            case 'n':
                value.push_back('\n');
                break;
            case 't':
                value.push_back('\t');
                break;
            case '\\':
                value.push_back('\\');
                break;
            case '"':
                value.push_back('"');
                break;
            default:
                // TODO: db.SyntaxError();
                break;
            }
        }
        else
            value.push_back(literal[i]);
    }
    
    return GetStringId(value.c_str());
}

bool Database::UserErrorReported() const
{
    return NumberOfErrors()>0;
}

void DatabaseImpl::ReportUserError()
{
    ++errorCount;
}

Entity Database::AddStrings(int id1, int id2)
{
    return CreateString(GetString(id1) + GetString(id2));
}

int DatabaseImpl::GetStringId(const string_type &str)
{
    return datastore->strings.GetId(str);
}

int DatabaseImpl::GetAtStringId(const string_type &str)
{
    return datastore->atstrings.GetId(str);
}

int DatabaseImpl::GetStringId(const char *str)
{
    return datastore->strings.GetId(string_type(str, datafile.data()));
}

int DatabaseImpl::GetAtStringId(const char *str)
{
    return datastore->atstrings.GetId(string_type(str, datafile.data()));
}


int Database::ReadFile(const char *filename)
{
    FILE * f = fopen(filename, "r");

    if(f)
    {
        yyscan_t scanner;

        yylex_init_extra(this, &scanner);

        yyset_in(f, scanner);
        yyrestart(f, scanner);
        int p;
        try
        {
            p = yyparse(scanner, *this);
        }
        catch(std::bad_alloc&)
        {
            Error("Memory limit reached");
        }
        catch(std::length_error&)
        {
            Error("Memory limit reached");
        }
        catch(...)
        {
            Error("Internal error - uncaught exception");
        }
        fclose(f);
        if(p) return 128;
        yylex_destroy(scanner);
        return 0;
    }

    return 1;
}

std::shared_ptr<Relation> DatabaseImpl::GetRelation(const CompoundName &cn)
{
    auto t = datastore->tables.find(cn);
    
    if(t != datastore->tables.end()) return t->second;
    
    // Find all tables that contain this one:
    /*
     Algorithm:
     for each name, look up names in "names", and create a set.
     This finds all possibly-related names.
     */
    
    std::unordered_set<CompoundName, CompoundName::Hash> subsets, supersets;
    
    for(auto i : cn.parts)
    {
        auto m = datastore->names.equal_range(i);
        for(auto j=m.first; j!=m.second; ++j)
        {
            if(cn <= j->second) supersets.insert(j->second);
            if(j->second <= cn) subsets.insert(j->second);
        }
    }
    
    // Create the appropriate mappings to the subsets
    std::shared_ptr<Relation> relation;
    
    relation = allocate_shared<Predicate>(datafile, *this, cn, cn.parts.size()+1, BindingType::Unbound);
    
    datastore->tables[cn] = relation;

    for(auto & superset : supersets)
    {
        CreateProjection(superset, cn);
    }

    for(auto & subset : subsets)
    {
        CreateProjection(cn, subset);
    }
    
    for(auto i : cn.parts) datastore->names.insert(std::make_pair(i, cn));
    return relation;
}

std::size_t Database::GlobalCallCount()
{
    return Evaluation::GlobalCallCount();
}

std::size_t Database::GlobalCallCountLimit()
{
    return Evaluation::GetGlobalCallCountLimit();
}


void DatabaseImpl::CreateProjection(const CompoundName &from, const CompoundName &to)
{
    /*
    std::cout << "Create a projection from ";
    for(auto a : from.parts)
        std::cout << GetString(a) << "-" << a << " ";
    std::cout << "to ";
    for(auto a : to.parts)
        std::cout << GetString(a) << "-" << a << " ";
    std::cout << std::endl;
    */
    
    // Map from input positions to output positions.
    std::vector<int> projection(to.parts.size()+1);
    std::vector<int> cols(from.parts.size()+1);
    
    for(int i=0; i<=from.parts.size(); ++i) cols[i] = i;
    
    for(int i=0, j=0; j<to.parts.size(); ++j)
    {
        while(from.parts[i] < to.parts[j]) ++i;
        projection[j+1] = i+1;
    }
    
    auto writer = allocate_shared<Writer>(datafile, datastore->tables[to], projection);
    
    auto reader = allocate_shared<Reader>(datafile, datastore->tables[from], cols, writer);
    
    auto eval = allocate_shared<RuleEvaluation>(datafile, cols.size(), reader);
    
    datastore->tables[to]->AddRule(eval);
}

int DatabaseImpl::NumberOfErrors() const
{
    return errorCount;
}

int DatabaseImpl::NumberOfResults() const
{
    return resultCount;
}

void Database::WarningEmptyRelation(Relation & relation)
{
    if(&relation != &GetQueryRelation())
    {
//    ++errorCount;
        std::cerr << Colours::Error << "Warning: Querying empty relation '" << GetString(relation.Name()) << "/" << relation.Arity() << "'\n" << Colours::Normal;
    }
}

void DatabaseImpl::RunQueries()
{
    class QueryVisitor : public Receiver
    {
    public:
        QueryVisitor(DatabaseImpl & db, int arity, const CompoundName & cn) : database(db), arity(arity), cn(cn), sortedRow(arity) {}
        
        void OnRow(Entity * row) override
        {
            sortedRow[0] = row[0];
            for(int i=1; i<arity; ++i)
                sortedRow[i] = row[cn.mapFromInputToOutput[i-1]+1];
            database.AddResult(sortedRow.data(), arity, false);
        }
        DatabaseImpl & database;
        
        const int arity;
        const CompoundName & cn;
        std::vector<Entity> sortedRow;
    };
        
    class Visitor : public Receiver
    {
    public:
        Visitor(DatabaseImpl & db) : database(db) {}
        std::size_t queries = 0;
        DatabaseImpl & database;

        void OnRow(Entity * data) override
        {
            ++queries;
            std::cout << Colours::Relation;
            database.Print(data[0], std::cout);
            std::cout << Colours::Normal << " has results:\n";
            //Entity queryName = data[0];

            // Join with all attributes in the
            database.datastore->queryPredicate->VisitAttributes([&](Relation&r) {
                QueryVisitor qv(database, r.Arity(), *r.GetCompoundName());
                std::vector<Entity> row(r.Arity());
                row[0] = data[0];
                r.Query(&row[0], 1, qv);
            });
            
        }
    } visitor(*this);
    
    datastore->queryPredicate->Query(nullptr, 0, visitor);
}

void DatabaseImpl::AddResult(const Entity * row, int arity, bool displayFirstColumn)
{
    ++resultCount;
    
    std::cout << "\t";
    bool isFirst = true;
    for(int i=displayFirstColumn ? 0 : 1; i<arity; ++i)
    {
        if (isFirst)
            isFirst = false;
        else
            std::cout << ", ";
        std::cout << Colours::Value;
        Print(row[i], std::cout);
        std::cout << Colours::Normal;
    }
    std::cout << std::endl;
}

bool DatabaseImpl::AnsiHighlightingEnabled() const
{
    return true;
}

void DatabaseImpl::SetExpectedResults(int count)
{
    expectedResults = count;
}

void Database::Error(const char * msg)
{
    ReportUserError();
    std::cerr << Colours::Error << msg << Colours::Normal << std::endl;
}

void Database::SetEvaluationLimit(std::size_t limit)
{
    Evaluation::SetGlobalCallCountLimit(limit);
}

void DatabaseImpl::CheckErrors()
{
    if(Evaluation::GlobalCallCount() > Evaluation::GetGlobalCallCountLimit())
    {
        ReportUserError();
        std::cerr << Colours::Error << "Error: Evaluation terminated because step limit (" << Evaluation::GetGlobalCallCountLimit() << ") is reached\n" << Colours::Normal;
    }
    
    if(expectedResults != -1 && NumberOfResults() != expectedResults)
    {
        ReportUserError();
        std::cerr << Colours::Error << "Error: Did not get the expected number of results (" << expectedResults << ")\n" << Colours::Normal;
    }
}

void Database::ParityError(Relation & relation)
{
    ReportUserError();
    std::cerr << Colours::Error << "Error: predicate ";
    Evaluation::OutputRelation(std::cerr, *this, relation);
    std::cerr << Colours::Error << " has negative recursion\n" << Colours::Normal;
}

Relation & DatabaseImpl::GetQueryRelation() const
{
    return *datastore->queryPredicate;
}

void DatabaseImpl::SetAnsiColours(bool enabled)
{
    if(enabled)
    {
        Colours::Normal = "\033[0m";
        Colours::Value = "\033[1;35m";  // Light magenta
        Colours::Variable = "\033[0;32m"; // Green
        Colours::IntroducedVariable = "\033[1;32m"; // Light green
        Colours::Success = "\033[1;32m"; // Light green
        Colours::Relation = "\033[0;33m"; // Brown orange
        Colours::Error = "\033[1;31m"; // Light red
        Colours::Detail = "\033[1;30m"; // Light red
    }
    else
    {
        Colours::Normal = "";
        Colours::Value = "";
        Colours::Variable = "";
        Colours::IntroducedVariable = "";
        Colours::Success = "";
        Colours::Relation = "";
        Colours::Error = "";
        Colours::Detail = "";
    }
}


const char * Colours::Normal = "\033[0m";
const char * Colours::Value = "\033[1;35m";  // Light magenta
const char * Colours::Variable = "\033[0;32m"; // Green
const char * Colours::IntroducedVariable = "\033[1;32m"; // Light green
const char * Colours::Success = "\033[1;32m"; // Light green
const char * Colours::Relation = "\033[0;33m"; // Brown orange
const char * Colours::Error = "\033[1;31m"; // Light red
const char * Colours::Detail = "\033[1;30m"; // Light red

DataStore::DataStore(persist::shared_memory & mem) :
    strings(mem),
    atstrings(mem),
    unaryRelations({}, std::hash<RelationId>(), std::equal_to<RelationId>(), mem),
    binaryRelations({}, std::hash<RelationId>(), std::equal_to<RelationId>(), mem),
    reachesRelations({}, std::hash<RelationId>(), std::equal_to<RelationId>(), mem),
    relations({}, RelationHash(), std::equal_to<std::pair<RelationId, Arity>>(), mem),
    tables({}, CompoundName::Hash(), std::equal_to<CompoundName>(), mem),
    names({}, std::hash<StringId>(), std::equal_to<StringId>(), mem)
{
}

persist::shared_memory &DatabaseImpl::Storage()
{
    return datafile.data();
}

Optimizer & DatabaseImpl::GetOptimizer() const
{
    return optimizer;
}

Entity DatabaseImpl::NewEntity()
{
    return Entity { EntityType::NewType, datastore->entityCounter++ };
}

void DatabaseImpl::AddRelation(const std::shared_ptr<Relation> & rel)
{
    switch(rel->Arity())
    {
        case 1:
            datastore->unaryRelations[rel->Name()] = rel;
            datastore->relations[std::make_pair(rel->Name(),1)] = rel;
            break;
        case 2:
            datastore->binaryRelations[rel->Name()] = rel;
            datastore->relations[std::make_pair(rel->Name(),2)] = rel;
            // Fall through to the next case
        default:
            datastore->tables[*rel->GetCompoundName()] = rel;
            break;
    }
}
