#include "Database.hpp"
#include "Evaluation.hpp"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "Analysis.hpp"

#include <iostream>

void UnaryTable::Add(const Entity *row)
{
    MakeDirty();
    //std::cout << "Added (" << (int)e.type << "," << e.i << ") to the table\n";
    values.insert(row[0]);
}

void BinaryTable::Add(const Entity * row)
{
    MakeDirty();
    
    auto value = std::make_pair(row[0],row[1]);
    //std::cout << "Added (" << (int)e1.type << "," << e1.i << ") (" << (int)e2.type << "," << e2.i << ") to the table\n";
    auto p = values.insert(value);
    
    if(p.second)
    {
        // TODO: Lazy map initialization
        map1.insert(value);
        map2.insert(std::make_pair(row[1], row[0]));
    }
}

std::size_t UnaryTable::Count()
{
    return values.size();
}

std::size_t BinaryTable::Count()
{
    return values.size();
}

PrintRelation::PrintRelation(std::ostream & output, Database &db, int name) :
    SpecialPredicate(db, name), output(output)
{
}

void PrintRelation::Add(const Entity * row)
{
    database.Print(row[0], output);
    output << std::endl;
}

void ErrorRelation::Add(const Entity * row)
{
    output << Colours::Error << "Error: ";
    PrintRelation::Add(row);
    output << Colours::Normal;
    database.ReportUserError();
}

void SpecialPredicate::AddRule(const std::shared_ptr<Evaluation> & eval)
{
    // Run the rule immediately.
    eval->Evaluate(nullptr);
    if( database.Explain() )
        eval->Explain(database, std::cout, 0);
}

ExpectedResults::ExpectedResults(Database &db, RelationId name) : SpecialPredicate(db, name)
{
}

void ExpectedResults::Add(const Entity * data)
{
    if(data->type != EntityType::Integer)
        database.Error("Invalid type for expected-results");
    
    database.SetExpectedResults(data->i);
}

void EvaluationStepLimit::Add(const Entity * data)
{
    if(data->type != EntityType::Integer)
        database.Error("Invalid type for evaluation-step-limit");
    
    database.SetEvaluationLimit(data->i);
}

std::size_t SpecialPredicate::Count()
{
    return 0;
}

ErrorRelation::ErrorRelation(Database &db) : PrintRelation(std::cout, db, db.GetStringId("error"))
{
}

std::size_t Table::Count()
{
    return hash.size();
}

void UnaryTable::Query(Entity * row, int columns, Visitor &v)
{
    RunRules();
    
    switch(columns)
    {
        case 0:
            for(auto &i : values)
            {
                v.OnRow(&i);
            }
            break;
        case 1:
            {
                auto i = values.find(row[0]);
                if (i != values.end())
                    v.OnRow(row);
                break;
            }
            break;
        default:
            assert(!"Invalid query columns");
    }
}

void SpecialPredicate::Query(Entity * row, int, Visitor&)
{
    // Empty relation.
}

void BinaryTable::Query(Entity * row, int bound, Visitor&v)
{
    RunRules();
    
    switch(bound)
    {
        case 0:
            for(auto & i : values)
            {
                Entity data[2];
                // TODO: Store a pair in the table to make the scan faster
                data[0] = i.first;
                data[1] = i.second;
                v.OnRow(data);
            }
            break;
        case 1:
            {
                auto range = map1.equal_range(row[0]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[1] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 2:
            {
                auto range = map2.equal_range(row[1]);
                for(auto i=range.first; i!=range.second; ++i)
                {
                    row[0] = i->second;
                    v.OnRow(row);  // ??
                }
            }
            break;
        case 3:
            {
                if(values.find(std::make_pair(row[0], row[1])) != values.end())
                    v.OnRow(row);
            }
            break;
        default:
            std::cout << "TODO: Implement the join\n";
            // Not implemented yet
            break;
    }
    // todo
}


Predicate::Predicate(Database &db, int name) :
    rulesRun(false), name(name), database(db),
    evaluating(false), recursive(false), sizeAtLastRecursiveCall(0)
{
}

void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    rules.push_back(rule);
    if(rulesRun)
    {
        // ?? What happens if it's recursive??
        // Generally it isn't as it came from a projection.
        rule->Evaluate(nullptr);
    }
    MakeDirty();
}

void Predicate::MakeDirty()
{
    rulesRun = false;
}

void Predicate::RunRules()
{
    if(rulesRun) return;
    
    AnalysePredicate(database, *this);
    
    if(evaluating)
    {
        if(!recursive)
        {
            recursive = true;
            sizeAtLastRecursiveCall = Count();
        }
        return;
    }
        
    evaluating = true;
    
    std::size_t iteration = 1;
    

    do
    {
        sizeAtLastRecursiveCall = Count();
        recursive = false;
        for(auto & p : rules)
        {
            if(iteration == 1 || p->onRecursivePath)
                p->Evaluate(nullptr);
        }
        ++iteration;
    }
    while (recursive && Count()>sizeAtLastRecursiveCall);
    
    evaluating = false;
    rulesRun = true;
    if(database.Explain())
    {
        // if(Relation::recursive) std::cout << Colours::Error << "Recursive\n" << Colours::Normal;
        // if(onRecursivePath) std::cout << "On recursive path\n";
        std::cout << "Evaluated " << rules.size() << " rule" << (rules.size()>1 ? "s" : "") << " in ";
        Evaluation::OutputRelation(std::cout, database, *this);
        
        if(Relation::recursive || onRecursivePath)
        {
            std::cout << Colours::Detail << " (Flags:";
            if(Relation::recursive) std::cout << "R";
            if(Relation::onRecursivePath) std::cout << "r";
            std::cout << ")" << Colours::Normal;
        }
        
        std::cout << " ->\n";
        for(auto & p : rules)
        {
            p->Explain(database, std::cout, 4);
        }
    }
}

bool Predicate::HasRules() const
{
    return !rules.empty();
}

UnaryTable::UnaryTable(Database &db, int name) : Predicate(db, name)
{
}

BinaryTable::BinaryTable(Database &db, int name) : Predicate(db, name)
{
}

SpecialPredicate::SpecialPredicate(Database &db, int name) : Predicate(db, name)
{
}

EvaluationStepLimit::EvaluationStepLimit(Database &db, RelationId name) : SpecialPredicate(db, name)
{
}

int Predicate::Name() const
{
    return name;
}

Table::Comparer::Comparer(const std::vector<Entity> & base, int arity, int mask) : base(base), arity(arity), mask(mask)
{    
}

std::size_t Relation::GetCount()
{
    RunRules();
    return Count();
}

int UnaryTable::Arity() const
{
    return 1;
}

int BinaryTable::Arity() const
{
    return 2;
}

int SpecialPredicate::Arity() const
{
    return 1;
}

void Predicate::AddAttribute(const std::shared_ptr<Relation> & attribute)
{
    attributes.insert(attribute);
}

void Predicate::VisitAttributes(const std::function<void(Relation&)> & visitor) const
{
    for(auto &r : attributes) visitor(*r);
}

const CompoundName * Relation::GetCompoundName() const
{
    return nullptr;
}

void Predicate::VisitRules(const std::function<void(Evaluation&)>&fn) const
{
    for(auto & rule : rules)
        fn(*rule);
}
