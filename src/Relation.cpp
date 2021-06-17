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

class EmptyReceiver : public Receiver
{
    void OnRow(Entity * row) override
    {
        
    }
};

void Predicate::Reset()
{
    loop.reset();
    rules.rules.clear();
    rulesRun = false;
    analysed = false;
    analysedForRecursion = false;
    recursiveDepth = -1;
    recursiveRoot = -1;
}

void SpecialPredicate::AddRule(const std::shared_ptr<Evaluation> & eval)
{
    // Run the rule immediately.
    // ?? What about analysis ??

    Reset();
    Predicate::AddRule(eval);
    EmptyReceiver receiver;
    Predicate::Query(nullptr, 0, receiver);
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

void SpecialPredicate::Query(Row, Columns, Receiver&)
{
    // Empty relation.
}

void SpecialPredicate::QueryDelta(Row row, Columns columns, Receiver &v)
{
    Query(row, columns, v);
}


Predicate::Predicate(Database &db, RelationId name, ::Arity arity, bool reaches, BindingType binding, Columns cols) :
    rulesRun(false), name(name), database(db),
    attributes({}, std::hash<Relation*>(), std::equal_to<Relation*>(), db.Storage()),
    reaches(reaches), bindingPredicate(binding), bindingColumns(cols),
    rules(db)
{
    table = allocate_shared<TableImpl>(db.Storage(), db.Storage(), arity);
#if !NDEBUG
    debugName = db.GetString(name).c_str();
#endif
}

Predicate::Predicate(Database &db, const CompoundName & cn, ::Arity arity, BindingType binding, Columns cols) :
    rulesRun(false), name(cn.parts[0]), database(db),
    compoundName(cn),
    attributes({}, std::hash<Relation*>(), std::equal_to<Relation*>(), db.Storage()),
    reaches(false),
    bindingPredicate(binding), bindingColumns(cols),
    rules(db)
{
    table = allocate_shared<TableImpl>(db.Storage(), db.Storage(), arity);
#if !NDEBUG
    debugName = db.GetString(name).c_str();
#endif
}

bool Predicate::IsReaches() const
{
    return reaches;
}

BindingType Predicate::GetBinding() const
{
    return bindingPredicate;
}

Columns Predicate::GetBindingColumns() const
{
    return bindingColumns;
}


void Predicate::AddRule(const std::shared_ptr<Evaluation> & rule)
{
    auto p = rule;
    AnalyseRule(database, p);
    rules.Add(p);
}

void Predicate::RunRules()
{
    AnalysePredicate(database, *this);

    assert(loop);
    loop->RunRules();
}

bool Predicate::HasRules() const
{
    return !rules.rules.empty();
}

SpecialPredicate::SpecialPredicate(Database &db, int name) : Predicate(db, name, 1, false, BindingType::Unbound, 0)
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

void Predicate::AddAttribute(Relation & attribute)
{
    attributes.insert(&attribute);
}

void Predicate::VisitAttributes(const std::function<void(Relation&)> & visitor) const
{
    for(auto &r : attributes) visitor(*r);
}

const CompoundName * Relation::GetCompoundName() const
{
    return nullptr;
}

void RuleSet::SetRecursiveRules(const std::shared_ptr<Evaluation> & baseCase, const std::shared_ptr<Evaluation> & recursiveCase)
{
    rules.clear();
    rules.push_back(baseCase);
    rules.push_back(recursiveCase);
    baseCase->onRecursivePath = false;
    recursiveCase->onRecursivePath = true;
}

Arity Predicate::Arity() const { return table->GetArity(); }

Size Predicate::Count() { return table->Rows(); }

void Predicate::Query(Row row, Columns columns, Receiver &r)
{
    RunRules();
    table->Query(row, columns, r);
}

void Predicate::QueryDelta(Entity * row, Columns columns, Receiver &r)
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

void Strlen::Query(Row row, Columns columns, Receiver&v)
{
    switch(columns.mask)
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

void BuiltinFnPredicate::QueryDelta(Row row, Columns columns, Receiver&v) {}

int BuiltinFnPredicate::Arity() const { return arity; }

BuiltinFnPredicate::BuiltinFnPredicate(Database & database, const CompoundName & cn) : Predicate(database, cn, cn.parts.size()+1, BindingType::Unbound, 0), arity(cn.parts.size()+1)
{
}

Lowercase::Lowercase(Database &db) : BuiltinFnPredicate(db, std::vector<int> { db.GetStringId("lowercase") })
{
}

Uppercase::Uppercase(Database &db) : BuiltinFnPredicate(db, std::vector<int> { db.GetStringId("uppercase") })
{
}

void Lowercase::Query(Row row, Columns columns, Receiver&v)
{
    switch(columns.mask)
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

void Uppercase::Query(Row row, Columns columns, Receiver&v)
{
    switch(columns.mask)
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

Relation& Predicate::GetBindingRelation(Columns columns)
{
    auto &i = bindingRelations[columns];
    
    if(!i)
    {
        // The "arity" is the number of bits set in columns.
        int arity=0;
        auto m = columns.mask;
        while(m)
        {
            if(m &1) ++arity;
            m>>=1;
        }
        i = compoundName.parts.size()>0 ?
        std::make_shared<Predicate>(database, compoundName, arity, BindingType::Binding, columns) :
        std::make_shared<Predicate>(database, name, arity, reaches, BindingType::Binding, columns);
    }
    
    return *i;
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
    
    void UpdateWrites(Evaluation & rule, Relation & newPredicate)
    {
        rule.VisitWrites([&](Relation *& rel, int n, const int * p) {
            if(rel == &targetPredicate)
            {
                rel = &newPredicate;
            }
        });

        rule.VisitNext([&](EvaluationPtr & next, bool) {
            UpdateWrites(*next, newPredicate);
        });
    }
private:
    void AnalyseWrites(Evaluation & eval)
    {
        eval.VisitWrites([&](Relation *& relation, int len, const int* p){
            if (relation == &targetPredicate && arguments.empty())
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


std::shared_ptr<Evaluation> MakeBoundRule(const std::shared_ptr<Evaluation> & rule, Predicate & oldPredicate, Relation & bindingRelation, Columns columns)
{
    WriteAnalysis write(*rule, oldPredicate);

    auto clone = rule->Clone();
    write.UpdateVariables(*clone);
    write.UpdateWrites(*clone, bindingRelation);

    return clone;
}

Relation& Predicate::GetBoundRelation(Columns columns)
{
    auto &i = boundRelations[columns];
    
    if(!i)
    {
        i = compoundName.parts.size()>0 ?
            std::make_shared<Predicate>(database, compoundName, Arity(), BindingType::Bound, columns) :
            std::make_shared<Predicate>(database, name, Arity(), reaches, BindingType::Bound, columns);

        // TODO: Create the bound version of the rule.
        std::vector<int> boundArguments;
        
        auto & binding = GetBindingRelation(columns);
        for(auto &r : rules.rules)
        {
            auto b = MakeBoundRule(r, *this, *i, columns);
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
        
        table->ReadAllData(adder);
    }
    return *i;
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

void ExecutionUnit::AddRelation(Relation & rel)
{
    relations.insert(&rel);
//    rel.VisitSteps([&](EvaluationPtr & step) {
//        step->VisitWrites([&](std::weak_ptr<Relation>&rel, int, const int*) {
//            relations.insert(&*rel.lock());
//        });
//    });
    
    rel.VisitRules([&](EvaluationPtr & rule) {
        rules.rules.push_back(rule);
    });
}

void ExecutionUnit::RunRules()
{
    if(evaluated) return;
    evaluated = true;
    
    for(auto r : relations)
        r->FirstIteration();

    for(auto & p : rules.rules)
    {
        p->Evaluate(nullptr);
    }

    Size loopSize;
    
    bool resultsFound;
    do
    {
        for(auto r : relations)
            r->NextIteration();

        loopSize = numberOfResults;

        if(database.GetVerbosity()>2)
        {
            std::cout << "Number of results in loop = " << loopSize << std::endl;
            Explain();
        }
        
        for(auto & p : rules.rules)
        {
            if(p->onRecursivePath)
                p->Evaluate(nullptr);
        }
        resultsFound = numberOfResults > loopSize;
        loopSize = numberOfResults;
    }
    while(resultsFound);

    for(auto r : relations)
        r->NextIteration();
    
    if(database.Explain())
        Explain();
}

void ExecutionUnit::Explain()
{
    std::cout << "Evaluated ";
    bool first = true;
    for(auto & r : relations)
    {
        if(first) first = false; else std::cout << ", ";
        Evaluation::OutputRelation(std::cout, database, *r);
    }
        
    std::cout << " ->\n";
    for(auto & p : rules.rules)
    {
        p->Explain(database, std::cout, 4);
    }
}

ExecutionUnit::ExecutionUnit(Database & db) : database(db), rules(db)
{
}

void Predicate::FirstIteration()
{
    table->FirstIteration();
}

void Predicate::NextIteration()
{
    table->NextIteration();
}

void RuleSet::Add(const EvaluationPtr & rule)
{
    rules.push_back(rule);
}

void Predicate::VisitRules(const std::function<void(Evaluation&)>&fn)
{
    for(auto & rule : rules.rules)
        fn(*rule);
}

void Predicate::VisitRules(const std::function<void(EvaluationPtr&)>&fn)
{
    for(EvaluationPtr & rule : rules.rules)
        fn(rule);
}

RuleSet::RuleSet(Database &db) : rules(db.Storage())
{
}

void Evaluation::VisitSteps(EvaluationPtr & ptr, const std::function<void(EvaluationPtr&)> & fn)
{
    auto ptr2 = ptr;
    fn(ptr);
    ptr2->VisitNext([&](EvaluationPtr & next, bool) {
        VisitSteps(next, fn); });
}

void RuleSet::VisitRules(const std::function<void(EvaluationPtr&)> & fn)
{
    for(auto & r : rules)
        fn(r);
}

void RuleSet::VisitRules(const std::function<void(Evaluation&)> & fn)
{
    for(auto & r : rules)
        fn(*r);
}

void RuleSet::VisitSteps(const std::function<void(EvaluationPtr&)> & fn)
{
    for(auto & r : rules)
        Evaluation::VisitSteps(r, fn);
}

bool ExecutionUnit::Recursive() const
{
    return relations.size()>1;
}
