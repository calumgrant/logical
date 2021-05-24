#include "Database.hpp"
#include "Evaluation.hpp"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "Analysis.hpp"

#include <iostream>



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

void SpecialPredicate::Query(Entity * row, int, Receiver&)
{
    // Empty relation.
}

void SpecialPredicate::QueryDelta(Entity * row, int columns, Receiver &v)
{
    Query(row, columns, v);
}


Predicate::Predicate(Database &db, int name) :
    rulesRun(false), name(name), database(db),
    evaluating(false), recursive(false)
{
}

void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    rules.push_back(rule);
    if(rulesRun)
    {
        // ?? What happens if it's recursive??
        // Generally it isn't as it came from a projection.
        // !! Projections can be recursive!
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
    if(!loop || loopResults == loop->numberOfResults)
        if(rulesRun) return;
    
    AnalysePredicate(database, *this);
    
    if(evaluating)
    {
        // Check we are in a recursive loop, otherwise something has gone badly wrong.
        assert(loop);
        
        if(!recursive)
        {
            recursive = true;
        }
        return;
    }
        
    evaluating = true;
    recursive = false;

    FirstIteration();

    for(auto & p : rules)
    {
        p->Evaluate(nullptr);
    }
    NextIteration();

    if(loop)
    {
        Size loopSize;
        
        bool resultsFound;
        do
        {
            recursive = false;
            loopSize = loop->numberOfResults;
            for(auto & p : rules)
            {
                if(p->onRecursivePath)
                    p->Evaluate(nullptr);
            }
            resultsFound = loop->numberOfResults > loopSize;
            loopSize = loop->numberOfResults;
            NextIteration();
        }
        while(resultsFound);
        loopResults = loop->numberOfResults;
    }

    NextIteration();

    evaluating = false;
    rulesRun = true;
    
    if(database.Explain())
    {
        std::cout << "Evaluated " << rules.size() << " rule" << (rules.size()!=1 ? "s" : "") << " in ";
        Evaluation::OutputRelation(std::cout, database, *this);
        
        if(Relation::loop)
        {
            std::cout << Colours::Detail << " (Flags:";
            if(Relation::loop) std::cout << "R";
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

std::size_t Relation::GetCount()
{
    RunRules();
    return Count();
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

bool SpecialPredicate::NextIteration()
{
    return false;
}

void SpecialPredicate::FirstIteration()
{
}

