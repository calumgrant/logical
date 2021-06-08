#include "Database.hpp"
#include "Evaluation.hpp"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "Analysis.hpp"
#include "TableImpl.hpp"
#include "Helpers.hpp"
#include "EvaluationImpl.hpp"

#include <iostream>

Relation::~Relation()
{
}

PrintRelation::PrintRelation(Database &db, int name) :
    SpecialPredicate(db, name)
{
}

void PrintRelation::Add(const Entity * row)
{
    database.Print(row[0], std::cout);
    std::cout << std::endl;
}

void ErrorRelation::Add(const Entity * row)
{
    std::cerr << Colours::Error << "Error: ";
    database.Print(row[0], std::cerr);
    std::cerr << std::endl;
    std::cerr << Colours::Normal;
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
    if(!data->IsInt())
        database.Error("Invalid type for expected-results");
    
    database.SetExpectedResults((std::int64_t)*data);
}

void EvaluationStepLimit::Add(const Entity * data)
{
    if(data->Type() != EntityType::Integer)
        database.Error("Invalid type for evaluation-step-limit");
    
    database.SetEvaluationLimit((std::int64_t)*data);
}

std::size_t SpecialPredicate::Count()
{
    return 0;
}

ErrorRelation::ErrorRelation(Database &db) : PrintRelation(db, db.GetStringId("error"))
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


Predicate::Predicate(Database &db, RelationId name, ::Arity arity, bool reaches, BindingType binding) :
    rulesRun(false), name(name), database(db),
    evaluating(false), rules(db.Storage()),
    attributes({}, std::hash<std::shared_ptr<Relation>>(), std::equal_to<std::shared_ptr<Relation>>(), db.Storage()),
    reaches(reaches), bindingPredicate(binding)
{
    table = allocate_shared<TableImpl>(db.Storage(), db.Storage(), arity);
}

Predicate::Predicate(Database &db, const CompoundName & cn, ::Arity arity, BindingType binding) :
    rulesRun(false), name(cn.parts[0]), database(db),
    evaluating(false),
    compoundName(cn), rules(db.Storage()),
    attributes({}, std::hash<std::shared_ptr<Relation>>(), std::equal_to<std::shared_ptr<Relation>>(), db.Storage()),
    reaches(false),
    bindingPredicate(binding)
{
    table = allocate_shared<TableImpl>(db.Storage(), db.Storage(), arity);
}

bool Predicate::IsReaches() const
{
    return reaches;
}

BindingType Predicate::GetBinding() const
{
    return bindingPredicate;
}


void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    // This is to test the Clone function
//    rules.push_back(rule->Clone());
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
    AnalysePredicate(database, *this);
    
    if(!loop || loopResults == loop->numberOfResults)
        if(rulesRun) return;
    
    if(evaluating)
    {
        // Check we are in a recursive loop, otherwise something has gone badly wrong.
        assert(loop);
        return;
    }


    evaluating = true;

    // std::cout << "Predicate ";
    // Evaluation::OutputRelation(std::cout, database, *this);
    // std::cout << " first iteration\n";
    
    if(!runBaseCase)
    {
        runBaseCase = true;
        table->FirstIteration();

        for(auto & p : rules)
        {
            p->Evaluate(nullptr);
        }
    }
    table->NextIteration();
    runBaseCase = true;

    if(loop)
    {
        Size loopSize;
        
        bool resultsFound;
        do
        {
            // std::cout << "Predicate ";
            // Evaluation::OutputRelation(std::cout, database, *this);
            // std::cout << " next iteration\n";

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

SpecialPredicate::SpecialPredicate(Database &db, int name) : Predicate(db, name, 1, false, BindingType::Unbound)
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

void Predicate::VisitRules(const std::function<void(Evaluation&)>&fn)
{
    for(auto & rule : rules)
        fn(*rule);
}

void Predicate::VisitRules(const std::function<void(EvaluationPtr&)>&fn)
{
    for(EvaluationPtr & rule : rules)
        fn(rule);
}

void Predicate::SetRecursiveRules(const std::shared_ptr<Evaluation> & baseCase, const std::shared_ptr<Evaluation> & recursiveCase)
{
    rules.resize(2);
    rules[0] = baseCase;
    rules[1] = recursiveCase;
    baseCase->onRecursivePath = false;
    recursiveCase->onRecursivePath = true;
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

Database & Predicate::GetDatabase() const
{
    return database;
}

Strlen::Strlen(Database &db) : BuiltinFnPredicate(db, std::vector<int> { db.GetStringId("strlen") })
{
}

void BuiltinFnPredicate::AddRule(const std::shared_ptr<Evaluation> &)
{
}

std::size_t BuiltinFnPredicate::Count()
{
    return 0;
}

void Strlen::Query(Entity *row, int columns, Receiver&v)
{
    switch(columns)
    {
        case 1:
            if(row[0].IsString())
            {
                row[1] = Entity(EntityType::Integer, (std::int64_t)database.GetString((std::int64_t)row[0]).size());
                v.OnRow(row);
            }
            break;
        case 3:
            if(row[0].IsString() && row[1].IsInt() && (std::int64_t)row[1] == database.GetString((std::int64_t)row[0]).size())
            {
                v.OnRow(row);
            }
            break;
    }
}

void BuiltinFnPredicate::QueryDelta(Entity*row, int columns, Receiver&v) {}

int BuiltinFnPredicate::Arity() const { return arity; }

BuiltinFnPredicate::BuiltinFnPredicate(Database & database, const CompoundName & cn) : Predicate(database, cn, cn.parts.size()+1, BindingType::Unbound), arity(cn.parts.size()+1)
{
}

Lowercase::Lowercase(Database &db) : BuiltinFnPredicate(db, std::vector<int> { db.GetStringId("lowercase") })
{
}

Uppercase::Uppercase(Database &db) : BuiltinFnPredicate(db, std::vector<int> { db.GetStringId("uppercase") })
{
}

void Lowercase::Query(Entity *row, int columns, Receiver&v)
{
    switch(columns)
    {
        case 1:
            if(row[0].IsString())
            {
                auto str = database.GetString((std::int64_t)row[0]);
                for(auto &c : str)
                    c = std::tolower(c);
                row[1] = database.CreateString(str);
                v.OnRow(row);
            }
            break;
        case 3:
            if(row[0].IsString() && row[1].IsString())
            {
                auto str = database.GetString((std::int64_t)row[0]);
                for(auto &c : str)
                    c = std::tolower(c);
                
                if(database.GetStringId(str) == (std::int64_t)row[1])
                    v.OnRow(row);
            }
            break;
    }
}

void Uppercase::Query(Entity *row, int columns, Receiver&v)
{
    switch(columns)
    {
        case 1:
            if(row[0].IsString())
            {
                auto str = database.GetString((std::int64_t)row[0]);
                for(auto &c : str)
                    c = std::toupper(c);
                row[1] = database.CreateString(str);
                v.OnRow(row);
            }
            break;
        case 3:
            if(row[0].IsString() && row[1].IsString())
            {
                auto str = database.GetString((std::int64_t)row[0]);
                for(auto &c : str)
                    c = std::toupper(c);
                
                if(database.GetStringId(str) == (std::int64_t)row[1])
                    v.OnRow(row);
            }
            break;
    }

}

std::shared_ptr<Relation> Predicate::GetBindingRelation(int columns)
{
    auto &i = bindingRelations[columns];
    
    if(!i)
    {
        i = compoundName.parts.size()>0 ?
        std::make_shared<Predicate>(database, compoundName, Arity(), BindingType::Binding) :
        std::make_shared<Predicate>(database, name, Arity(), reaches, BindingType::Binding);
    }
    
    return i;
}

class WriteAnalysis
{
public:
    std::vector<int> arguments;
    
    WriteAnalysis(Evaluation & rule, Predicate & oldPredicate) : targetPredicate(oldPredicate)
    {
        AnalyseWrites(rule);
    }
    
    void UpdateVariables(Evaluation & rule)
    {
        if(variableMapping.empty()) return;
        
        rule.VisitVariables([&](int & variable, Evaluation::VariableAccess access) {
            auto m = variableMapping.find(variable);
            if(m != variableMapping.end())
                variable = m->second;
        });
        
        rule.VisitNext([&](EvaluationPtr & next, bool) {
            UpdateVariables(*next);
        });
    }
    
    void UpdateWrites(Evaluation & rule, const std::shared_ptr<Relation> & newPredicate)
    {
        rule.VisitWrites([&](std::weak_ptr<Relation> & rel, int n, const int * p) {
            auto r = rel.lock();
            if(&*r == &targetPredicate)
            {
                rel = newPredicate;
            }
        });

        rule.VisitNext([&](EvaluationPtr & next, bool) {
            UpdateWrites(*next, newPredicate);
        });
    }
private:
    void AnalyseWrites(Evaluation & eval)
    {
        eval.VisitWrites([&](std::weak_ptr<Relation>&rel, int len, const int* p){
            auto relation = rel.lock();
            if (&*relation == &targetPredicate && arguments.empty())
            {
                arguments.assign(p, p+len);
            }
            else
            {
                int i=0;
                eval.VisitVariables([&](int& arg, Evaluation::VariableAccess access) {
                    if(i<arguments.size() && arg != arguments[i])
                        variableMapping[arg] = arguments[i];
                    ++i;
                });
            }
                
        });
        
        eval.VisitNext([&](std::shared_ptr<Evaluation> & next, bool) {
            AnalyseWrites(*next);
        });
    }
    
    Predicate & targetPredicate;
    std::unordered_map<int, int> variableMapping;
};


std::shared_ptr<Evaluation> MakeBoundRule(const std::shared_ptr<Evaluation> & rule, Predicate & oldPredicate, const std::shared_ptr<Relation> & bindingRelation, int columns)
{
    WriteAnalysis write(*rule, oldPredicate);

    auto clone = rule->Clone();
    write.UpdateVariables(*clone);
    write.UpdateWrites(*clone, bindingRelation);

    return clone;
}

std::shared_ptr<Relation> Predicate::GetBoundRelation(int columns)
{
    auto &i = boundRelations[columns];
    
    if(!i)
    {
        i = compoundName.parts.size()>0 ?
            std::make_shared<Predicate>(database, compoundName, Arity(), BindingType::Bound) :
            std::make_shared<Predicate>(database, name, Arity(), reaches, BindingType::Bound);

        // TODO: Create the bound version of the rule.
        std::vector<int> boundArguments;
        
        auto binding = GetBindingRelation(columns);
        for(auto &r : rules)
        {
            auto b = MakeBoundRule(r, *this, i, columns);
            i->AddRule(b);
        }
        
        // Copy the data across already
        // TODO: Create a rule to transfer the data across.
        
        struct Adder : public Receiver
        {
            std::shared_ptr<Relation> table;
            void OnRow(Entity * row) override
            {
                table->Add(row);
            }
            Adder(std::shared_ptr<Relation>&t) : table(t) { }
        } adder { i };
        
        table->FirstIteration();
        table->Query(nullptr, 0, adder);
    }
    return i;
}

bool Predicate::IsSpecial() const
{
    return false;
}

bool BuiltinFnPredicate::IsSpecial() const
{
    return true;
}

bool SpecialPredicate::IsSpecial() const
{
    return true;
}
