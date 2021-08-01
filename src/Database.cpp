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
#include <sstream>

struct HashHelper
{
    int operator()(const std::pair<int, CompoundName> &v) const
    {
        return v.first + 101 * CompoundName::Hash()(v.second);
    }
};

struct VariadicExtern
{
    Logical::Extern fn;
    void * data;
};

class DataStore
{
public:
    DataStore(persist::shared_memory & memory, AllocatorData & alloc);
    
    StringTable strings, atstrings;
    
    using relmap = unordered_map_helper<PredicateName, std::shared_ptr<Relation>, PredicateName::Hash>::map_type;
    
    relmap relations, externs;

    // Names, indexed on their first column
    unordered_map_helper<StringId, PredicateName>::multimap_type nameParts;
    
    unordered_map_helper<StringId, VariadicExtern>::map_type variadicExterns;

    // List of imports so we don't import anything twice
    unordered_set_helper<StringId>::set_type imports;
    
    std::atomic<std::int64_t> entityCounter = 0;
    
    bool initialized = false;
};

void yyrestart (FILE *input_file ,yyscan_t yyscanner );
int yylex_init (yyscan_t* scanner);
int yylex_init_extra (ParseData *, yyscan_t* scanner);
int yylex_destroy (yyscan_t yyscanner );
void yyset_in  (FILE * in_str ,yyscan_t yyscanner );


void Database::UnboundError(const char *name, const SourceLocation & loc)
{
    std::stringstream ss;
    ss << "Variable '" << name << "' is unbound";
    Error(loc, ss.str().c_str());
}

DatabaseImpl::DatabaseImpl(Optimizer & optimizer, const char * name, int limitMB) :
    datafile(name, 2, 2, 1, 16384, limitMB * 1000000ll, name ? 0 : persist::temp_heap),
    datastore(datafile.data(), datafile.data(), Storage()),
    optimizer(optimizer),
    memoryCounter(limitMB * 1000000ll)
{

    if(!datastore->initialized)
    {
        datastore->initialized = true;
    }
    
    LoadModule("stdlib");
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

const char * DatabaseImpl::GetString(int id) const
{
    return datastore->strings.GetString(id);
}

const char * DatabaseImpl::GetAtString(int id) const
{
    return datastore->atstrings.GetString(id);
}

Relation& DatabaseImpl::GetRelation(const PredicateName & name)
{
    auto i = datastore->relations.find(name);

    if (i == datastore->relations.end())
    {
        if(name.objects.parts.size() == 1)
        {
            auto j = datastore->variadicExterns.find(name.objects.parts[0]);
            if(j != datastore->variadicExterns.end())
            {
                auto r = allocate_shared<ExternPredicate>(Storage(), *this, name);
                r->AddVarargs(j->second.fn, j->second.data);
                datastore->relations.insert(std::make_pair(name, r));
                return *r;
            }
        }
        
        auto r = allocate_shared<Predicate>(Storage(), *this, name);
        datastore->relations.insert(std::make_pair(name, r));
        if(name.reaches)
            MakeReachesRelation(*r);
        MakeProjections(*r);
        return *r;
    }
    else
        return *i->second;
}

void DatabaseImpl::MakeReachesRelation(Relation & rel)
{
    auto underlyingRelationName = rel.name;
    assert(underlyingRelationName.reaches);
    underlyingRelationName.reaches = false;
    auto & r = GetRelation(underlyingRelationName);

    {
        // Add the rule rel(_0,_1) :- r(_0, _1)
        std::vector<int> writeArgs = { 0, 1 };
        auto write = std::make_shared<Writer>(rel, writeArgs, SourceLocation{});
        auto reader = std::make_shared<Join>(r, std::vector<int>{-1,-1}, std::move(writeArgs), write);
        auto baseRule = std::make_shared<RuleEvaluation>(2, reader);
        rel.AddRule(baseRule);
    }
    
    {
        // Add the rule rel(_0, _1) :- rel(_0, _2), r(_2, _1).
        std::vector<int> writeArgs = { 0, 1 };
        auto write = std::make_shared<Writer>(rel, writeArgs, SourceLocation{});
        auto join2 = std::make_shared<Join>(r, std::vector<int> {2, -1}, std::vector<int> { -1, 1 }, write);
        auto join1 = std::make_shared<Join>(rel, std::vector<int> {-1,-1}, std::vector<int> {0, 2}, join2);
        auto recursiveRule = std::make_shared<RuleEvaluation>(3, join1);
        rel.AddRule(recursiveRule);
    }
}

void DatabaseImpl::Find(const PredicateName & name)
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
    GetRelation(name).Query(&row, 0, visitor);

    std::cout << "Found " << visitor.count << " results\n";
}

void Database::InvalidLhs()
{
    std::cerr << "Invalid left hand side of a rule.\n";
}

void DatabaseImpl::SetVerbosity(int v)
{
    verbosity = v;
}

int DatabaseImpl::GetVerbosity() const
{
    return verbosity;
}

bool Database::Explain() const
{
    return GetVerbosity()>1;
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
    return CreateString((std::string(GetString(id1)) + GetString(id2)).c_str());
}

int DatabaseImpl::GetStringId(const char *str)
{
    return datastore->strings.GetId(str);
}

int DatabaseImpl::GetAtStringId(const char *str)
{
    return datastore->atstrings.GetId(str);
}


int DatabaseImpl::ReadFile(const char *filename)
{
    int filenameId = GetStringId(filename);

    if(datastore->imports.find(filenameId) != datastore->imports.end())
        return 0; // Loaded already, so just ignore
    
    FILE * f = fopen(filename, "r");

    if(f)
    {
        datastore->imports.insert(filenameId);
        yyscan_t scanner;

        auto filenameId = GetStringId(filename);
        ParseData data { filenameId, *this };

        yylex_init_extra(&data, &scanner);

        yyset_in(f, scanner);
        yyrestart(f, scanner);
        int p;
        try
        {
            p = yyparse(scanner, data );
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

void DatabaseImpl::MakeProjections(Relation &relation)
{
    // Find all tables that contain this one:
    /*
     Algorithm:
     for each name, look up names in "names", and create a set.
     This finds all possibly-related names.
     */
    
    if(relation.name.IsDatalog()) return;
    
    std::unordered_set<PredicateName, PredicateName::Hash> subsets, supersets;
    
    // auto & cn = relation.name.attributes;
    auto & name = relation.name;
    
    for(auto i : name.attributes.parts)
    {
        auto m = datastore->nameParts.equal_range(i);
        for(auto j=m.first; j!=m.second; ++j)
        {
            if(name <= j->second) supersets.insert(j->second);
            if(j->second <= name) subsets.insert(j->second);
        }
    }

    for(auto i : name.objects.parts)
    {
        auto m = datastore->nameParts.equal_range(i);
        for(auto j=m.first; j!=m.second; ++j)
        {
            auto & debug = j->second;
            if(name <= j->second) supersets.insert(j->second);
            if(j->second <= name) subsets.insert(j->second);
        }
    }
    
    for(auto & superset : supersets)
    {
        CreateProjection(superset, relation.name);
    }

    for(auto & subset : subsets)
    {
        CreateProjection(relation.name, subset);
    }
    
    for(auto i : name.attributes.parts) datastore->nameParts.insert(std::make_pair(i, relation.name));
    for(auto i : name.objects.parts) datastore->nameParts.insert(std::make_pair(i, relation.name));
}

std::size_t Database::GlobalCallCount()
{
    return Evaluation::GlobalCallCount();
}

std::size_t Database::GlobalCallCountLimit()
{
    return Evaluation::GetGlobalCallCountLimit();
}


void DatabaseImpl::CreateProjection(const PredicateName &from, const PredicateName &to)
{
    /*
    std::cout << "Create a projection from ";
    from.Write(*this, std::cout);
    std::cout << " to ";
    to.Write(*this, std::cout);
    std::cout << std::endl;
     */
    
    // Map from input positions to output positions.
    std::vector<int> projection(to.attributes.parts.size()+1);
    std::vector<int> cols(from.attributes.parts.size()+1);
    
    for(int i=0; i<=from.attributes.parts.size(); ++i) cols[i] = i;
    
    for(int i=0, j=0; j<to.attributes.parts.size(); ++j)
    {
        while(from.attributes.parts[i] < to.attributes.parts[j]) ++i;
        projection[j+1] = i+1;
    }
    
    auto writer = allocate_shared<Writer>(Storage(), *datastore->relations[to], projection, SourceLocation{});
    
    std::vector<int> inputs(cols);
    std::fill(inputs.begin(), inputs.end(), -1);
    
    auto reader = allocate_shared<Join>(Storage(), *datastore->relations[from], inputs, cols, writer);
    
    auto eval = allocate_shared<RuleEvaluation>(Storage(), cols.size(), reader);
    
    datastore->relations[to]->AddRule(eval);
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
    std::cerr << Colours::Error << "Error: Querying undefined relation '";
    relation.name.Write(*this, std::cerr);
    std::cerr << "/" << relation.Arity() << "'\n" << Colours::Normal;
    ReportUserError();
}

void DatabaseImpl::AddResult(const Entity * row, int arity, bool displayFirstColumn)
{
    AddResult();
    
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

void DatabaseImpl::AddResult()
{
    ++resultCount;
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
    std::cerr << Colours::Error << "Error: " << msg << Colours::Normal << std::endl;
}

void Database::Error(const SourceLocation & loc, const char * msg)
{
    ReportUserError();
    std::cerr << Colours::Error << "Error at " << GetString(loc.filenameId) << ":" << loc.line << ":" << loc.column << ": " << msg << Colours::Normal << std::endl;
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

DataStore::DataStore(persist::shared_memory & mem, AllocatorData & alloc) :
    strings(alloc),
    atstrings(alloc),
    relations({}, PredicateName::Hash(), std::equal_to<PredicateName>(), alloc),
    nameParts({}, std::hash<StringId>(), std::equal_to<StringId>(), alloc),
    externs({}, PredicateName::Hash(), std::equal_to<PredicateName>(), alloc),
    variadicExterns({}, std::hash<StringId>(), std::equal_to<StringId>(), alloc),
    imports({}, std::hash<StringId>(), std::equal_to<StringId>(), alloc)
{
}

AllocatorData &DatabaseImpl::Storage()
{
#if MMAP_ALLOCATOR
    return datafile.data();
#else
    return memoryCounter;
#endif
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
    datastore->relations[rel->name] = rel;
}

Relation &DatabaseImpl::GetExtern(const PredicateName & pn)
{
    auto & rel = datastore->externs[pn];
    
    if(!rel)
    {
        rel = allocate_shared<ExternPredicate>(Storage(), *this, pn);
        AddRelation(rel);
    }
    
    return *rel;
}

void DatabaseImpl::Addvarargs(RelationId name, Logical::Extern fn, void * data)
{
    datastore->variadicExterns[name] = VariadicExtern { fn, data };
}

bool Database::LogRows() const
{
    return GetVerbosity()>=5;
}

persist::shared_memory & DatabaseImpl::SharedMemory()
{
    return datafile.data();
}

void DatabaseImpl::SetMemoryLimit(std::size_t size)
{
    datafile.data().limit(size);
    memoryCounter.limit(size);
}
