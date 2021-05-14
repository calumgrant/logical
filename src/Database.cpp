#include "Database.hpp"
#include "Evaluation.hpp"
#include "tokens.tab.h"

#include <iostream>

void yyrestart (FILE *input_file ,yyscan_t yyscanner );
int yylex_init (yyscan_t* scanner);
int yylex_init_extra (Database *, yyscan_t* scanner);
int yylex_destroy (yyscan_t yyscanner );
void yyset_in  (FILE * in_str ,yyscan_t yyscanner );

std::shared_ptr<Relation> Database::GetUnaryRelation(int name)
{
    auto i = unaryRelations.find(name);
    if (i == unaryRelations.end())
    {
        auto p = std::make_shared<UnaryTable>(*this, name);
        unaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
        return i->second;
}

std::shared_ptr<Relation> Database::GetBinaryRelation(int name)
{
    auto i = binaryRelations.find(name);
    if (i==binaryRelations.end())
    {
        auto p = std::make_shared<BinaryTable>(*this, name);
        binaryRelations.insert(std::make_pair(name, p));
        return p;
    }
    else
        return i->second;
}

void Database::UnboundError(const std::string &name, int line, int column)
{
    std::cerr << "Error at (" << line << ":" << column << "): " << name << " is unbound.\n";
}

Relation::~Relation()
{
}

Database::Database() : verbose(false), userError(false)
{
    int print = GetStringId("print");
    unaryRelations[print] = std::make_shared<PrintRelation>(std::cout, *this, print);
    unaryRelations[GetStringId("error")] = std::make_shared<ErrorRelation>(*this);
}

Database::~Database()
{
}

void Database::Print(const Entity &e, std::ostream &os) const
{
    switch(e.type)
    {
    case EntityType::None:
            os << "None";
            break;
    case EntityType::Integer:
        os << e.i;
        break;
    case EntityType::Float:
        os << e.f;
        break;
    case EntityType::Boolean:
        os << (e.i?"true":"false");
        break;
    case EntityType::String:
        os << GetString(e.i);
        break;
    case EntityType::AtString:
        os << "@" << GetAtString(e.i);
        break;
    case EntityType::Char:
    case EntityType::Byte:
        os << e.i;
        break;
    }
}

void Database::PrintQuoted(const Entity &e, std::ostream &os) const
{
    if(e.type == EntityType::String)
    {
        os << '\"';
        Print(e, os);
        os << '\"';
    }
    else
    {
        Print(e, os);
    }
}

const std::string &Database::GetString(int id) const
{
    return strings.GetString(id);
}

const std::string &Database::GetAtString(int id) const
{
    return atstrings.GetString(id);
}

std::shared_ptr<Relation> Database::GetRelation(int name, int arity)
{
    switch(arity)
    {
    case 1: return GetUnaryRelation(name);
    case 2: return GetBinaryRelation(name);
    }

    auto index = std::make_pair(name, arity);

    auto i = relations.find(index);

    if (i == relations.end())
    {
        auto r = std::make_shared<Table>(*this, name, arity);
        relations.insert(std::make_pair(index, r));
        return r;
    }
    else
        return i->second;
}

void Database::Find(int unaryPredicateName)
{
    class Tmp : public Relation::Visitor
    {
    public:
        Database &db;
        int count;
        Tmp(Database &db) : db(db), count() { }

        void OnRow(const Entity *e) override
        {
            std::cout << "\t";
            db.Print(*e, std::cout);
            std::cout << std::endl;
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

void Database::SetVerbose(bool v)
{
    verbose = v;
}

bool Database::Explain() const
{
    return verbose;
}

int Database::GetStringLiteral(const char *literal)
{
    int len = strlen(literal)-1;
    std::string value;
    // TODO: Do this in the scanner
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
    
    return GetStringId(value);
}

bool Database::UserErrorReported() const
{
    return userError;
}

void Database::ReportUserError()
{
    userError = true;
}

Entity Database::AddStrings(int id1, int id2)
{
    return CreateString(GetString(id1) + GetString(id2));
}

int Database::GetStringId(const std::string &str)
{
    return strings.GetId(str);
}

int Database::GetAtStringId(const std::string &str)
{
    return atstrings.GetId(str);
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
        int p = yyparse(scanner, *this);
        fclose(f);
        if(p) return 128;
        yylex_destroy(scanner);
    }

    return 0;
}

std::shared_ptr<Relation> Database::GetRelation(const CompoundName &cn)
{
    auto t = tables.find(cn);
    
    if(t != tables.end()) return t->second;
    
    // Find all tables that contain this one:
    /*
     Algorithm:
     for each name, look up names in "names", and create a set.
     This finds all possibly-related names.
     */
    
    std::unordered_set<CompoundName, CompoundName::Hash> subsets, supersets;
    
    for(auto i : cn.parts)
    {
        auto m = names.equal_range(i);
        for(auto j=m.first; j!=m.second; ++j)
        {
            if(cn <= j->second) supersets.insert(j->second);
            if(j->second <= cn) subsets.insert(j->second);
        }
    }
    
    // Create the appropriate mappings to the subsets
    auto relation = std::make_shared<Table>(*this, cn.parts[0], cn.parts.size()+1);
    
    tables[cn] = relation;

    for(auto & superset : supersets)
    {
        CreateProjection(superset, cn);
    }

    for(auto & subset : subsets)
    {
        CreateProjection(cn, subset);
    }
    
    for(auto i : cn.parts) names.insert(std::make_pair(i, cn));
    return relation;
}

std::size_t Database::GlobalCallCount()
{
    return Evaluation::GlobalCallCount();
}

void Database::CreateProjection(const CompoundName &from, const CompoundName &to)
{
    std::cout << "Create a projection from " << from.parts.size() << " to " << to.parts.size() << std::endl;
    
    // Map from input positions to output positions.
    std::vector<int> projection(to.parts.size());
    std::vector<int> cols(from.parts.size());
    
    for(int i=0; i<from.parts.size(); ++i) cols[i] = i;
    
    
    for(int i=0, j=0; j<to.parts.size(); ++j)
    {
        while(from.parts[i] < to.parts[j]) ++i;
        projection[j] = i;
    }
    
    auto writer = std::make_shared<Writer>(tables[to], projection);
    
    auto reader = std::make_shared<Reader>(tables[from], cols, writer);
    
    auto eval = std::make_shared<RuleEvaluation>(cols.size(), reader);
    
    tables[to]->AddRule(eval);
}
