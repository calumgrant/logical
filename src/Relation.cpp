#include "Database.hpp"
#include "Evaluation.hpp"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "Analysis.hpp"
#include "TableImpl.hpp"

#include <iostream>

Relation::~Relation()
{
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

void SpecialPredicate::Query(Entity * row, int, Receiver&)
{
    // Empty relation.
}

void SpecialPredicate::QueryDelta(Entity * row, int columns, Receiver &v)
{
    Query(row, columns, v);
}


Predicate::Predicate(Database &db, RelationId name, ::Arity arity) :
    rulesRun(false), name(name), database(db),
    evaluating(false), recursive(false)
{
    table = std::make_shared<TableImpl>(arity);
}

Predicate::Predicate(Database &db, const CompoundName & cn, ::Arity arity) :
    rulesRun(false), name(cn.parts[0]), database(db),
    evaluating(false), recursive(false),
    compoundName(cn)
{
    table = std::make_shared<TableImpl>(arity);
}


void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    rules.push_back(rule);
    if(rulesRun)
    {
        database.Error("Adding a rule to an evaluated relation is not supported (yet)");
        // ?? What happens if it's recursive??
        // Generally it isn't as it came from a projection.
        // !! Projections can be recursive!
        rule->Evaluate(nullptr);
        
        if(database.Explain())
        {
            rule->Explain(database, std::cout, 4);
        }
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

    table->FirstIteration();

    for(auto & p : rules)
    {
        p->Evaluate(nullptr);
    }
    table->NextIteration();

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
            table->NextIteration();
        }
        while(resultsFound);
        loopResults = loop->numberOfResults;
    }

    // Surely not needed any more?
    table->NextIteration();

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

SpecialPredicate::SpecialPredicate(Database &db, int name) : Predicate(db, name, 1)
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

Arity Predicate::Arity() const { return table->GetArity(); }

Size Predicate::Count() { return table->Rows(); }

void Predicate::Query(Entity * row, ColumnMask columns, Receiver &r)
{
    RunRules();
    table->Query(row, columns, r);
}

void Predicate::QueryDelta(Entity * row, ColumnMask columns, Receiver &r)
{
    RunRules();
    table->QueryDelta(row, columns, r);
}

void Predicate::Add(const Entity * data)
{
    table->loop = loop; // Hack :-(
    table->OnRow(const_cast<Entity*>(data));
}

const CompoundName * Predicate::GetCompoundName() const
{
    return compoundName.parts.empty() ? nullptr : &compoundName;
}
