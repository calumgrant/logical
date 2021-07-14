#include "Database.hpp"
#include "Evaluation.hpp"
#include "Colours.hpp"
#include "RelationImpl.hpp"
#include "Analysis.hpp"
#include "Helpers.hpp"
#include "EvaluationImpl.hpp"
#include "Table.hpp"

#include <iostream>
#include <sstream>

Relation::~Relation()
{
}

class EmptyReceiver : public Receiver
{
    void OnRow(Entity *row) override
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
    sealed = false;
}

void SpecialPredicate::AddRule(const std::shared_ptr<Evaluation> &eval)
{
    // Run the rule immediately.
    // ?? What about analysis ??

    Reset();
    Predicate::AddRule(eval);
    EmptyReceiver receiver;
    Predicate::Query(nullptr, 0, receiver);
}

std::size_t SpecialPredicate::Count()
{
    return 0;
}

void SpecialPredicate::Query(Row, Columns, Receiver &)
{
    // Empty relation.
}

void SpecialPredicate::QueryDelta(Row row, Columns columns, Receiver &v)
{
    Query(row, columns, v);
}

class BoolReceiver : public Receiver
{
public:
    bool hasResult = false;
    
    void OnRow(Row row) override
    {
        hasResult = true;
    }
};

bool SpecialPredicate::QueryExists(Row row, Columns columns)
{
    BoolReceiver receiver;
    Query(row, columns, receiver);
    return receiver.hasResult;
}

Predicate::Predicate(Database &db, const PredicateName & name) : rulesRun(false), database(db),
    rules(db)
{
    this->name = name;
    table = Table::MakeTable(db.Storage(), name.arity);
#if !NDEBUG
    std::ostringstream ss;
    name.Write(db, ss);
    debugName = ss.str();
#endif
}

void Predicate::AddRule(const std::shared_ptr<Evaluation> &rule)
{
    if(sealed)
    {
        // database.Error("Adding a rule to a sealed predicate");
        std::cout << "Warning: adding a rule to a sealed predicate\n";
        return;
    }
    assert(!sealed);
    auto p = rule;
    AnalyseRule(database, p);
    rules.Add(p);
    assert(!sealed);
}

void Predicate::RunRules()
{
    sealed = true;  // No more rules please.
    
    AnalysePredicate(database, *this);

    assert(loop);
    loop->RunRules();
}

bool Predicate::HasRules() const
{
    return !rules.rules.empty();
}

SpecialPredicate::SpecialPredicate(Database &db, const PredicateName & name) : Predicate(db, name)
{
}

std::size_t Relation::GetCount()
{
    RunRules();
    return Count();
}

void RuleSet::SetRecursiveRules(const std::shared_ptr<Evaluation> &baseCase, const std::shared_ptr<Evaluation> &recursiveCase)
{
    rules.clear();
    rules.push_back(baseCase);
    rules.push_back(recursiveCase);
    baseCase->onRecursivePath = false;
    recursiveCase->onRecursivePath = true;
}

Arity Relation::Arity() const { return name.arity; }

Size Predicate::Count() { return table->Rows(); }

void Predicate::Query(Row row, Columns columns, Receiver &r)
{
    RunRules();
    table->Query(row, columns, r);
}

void Predicate::QueryDelta(Entity *row, Columns columns, Receiver &r)
{
    RunRules();
    table->QueryDelta(row, columns, r);
}

bool Predicate::QueryExists(Row row, Columns columns)
{
    RunRules();
    return table->QueryExists(row, columns);
}

void Predicate::Add(const Entity *data)
{
    table->loop = loop; // Hack :-(
    table->OnRow(const_cast<Entity *>(data));
}

Database &Predicate::GetDatabase() const
{
    return database;
}

class WriteAnalysis
{
public:
    std::vector<int> arguments;

    WriteAnalysis(Evaluation &rule, Predicate &oldPredicate) : targetPredicate(oldPredicate)
    {
        AnalyseWrites(rule);
    }

    void UpdateVariables(Evaluation &rule)
    {
        if (variableMapping.empty())
            return;

        rule.VisitVariables([&](int &variable, Evaluation::VariableAccess access)
                            {
                                auto m = variableMapping.find(variable);
                                if (m != variableMapping.end())
                                    variable = m->second;
                            });

        rule.VisitNext([&](EvaluationPtr &next, bool)
                       { UpdateVariables(*next); });
    }

    void UpdateWrites(Evaluation &rule, Relation &newPredicate)
    {
        rule.VisitWrites([&](Relation *&rel, Columns n, const int *p)
                         {
                             if (rel == &targetPredicate)
                             {
                                 rel = &newPredicate;
                             }
                         });

        rule.VisitNext([&](EvaluationPtr &next, bool)
                       { UpdateWrites(*next, newPredicate); });
    }

private:
    void AnalyseWrites(Evaluation &eval)
    {
        eval.VisitWrites([&](Relation *&relation, int len, const int *p)
                         {
                             if (relation == &targetPredicate && arguments.empty())
                             {
                                 arguments.assign(p, p + len);
                             }
                             else
                             {
                                 int i = 0;
                                 eval.VisitVariables([&](int &arg, Evaluation::VariableAccess access)
                                                     {
                                                         if (i < arguments.size() && arg != arguments[i])
                                                             variableMapping[arg] = arguments[i];
                                                         ++i;
                                                     });
                             }
                         });

        eval.VisitNext([&](std::shared_ptr<Evaluation> &next, bool)
                       { AnalyseWrites(*next); });
    }

    Predicate &targetPredicate;
    std::unordered_map<int, int> variableMapping;
};

std::shared_ptr<Evaluation> MakeBoundRule(const std::shared_ptr<Evaluation> &rule, Predicate &oldPredicate, Relation &bindingRelation, Columns columns, Relation & binding2)
{
    WriteAnalysis write(*rule, oldPredicate);

    auto clone = rule->Clone();
    // write.UpdateVariables(*clone);
    write.UpdateWrites(*clone, bindingRelation);
    
    int ba = binding2.Arity();
    std::vector<int> inputs, outputs;
    inputs.reserve(ba);
    outputs.reserve(ba);
    
    for(int i=0; i<ba; ++i)
    {
        inputs.push_back(-1);
        outputs.push_back(i);
    }

    Evaluation::VisitSteps(clone, [&](EvaluationPtr &p) {
        auto re = std::dynamic_pointer_cast<RuleEvaluation>(p);
        if(re)
        {
            re->VisitNext([&](EvaluationPtr & next, bool) {
                for(int i=0; i<ba; ++i)
                    next->BindVariable(next, i);
                next = std::make_shared<Join>(binding2, inputs, outputs, next);
            });
        }
    });
    
    return clone;
}

Relation &Predicate::GetSemiNaivePredicate(Columns columns)
{
    auto &i = semiNaivePredicates[columns];
    sealed = true;

    if (!i)
    {
        i = std::make_shared<SemiNaivePredicate>(*this, columns, table);

        // std::vector<int> boundArguments;

        for (auto &r : rules.rules)
        {
            // auto b = MakeBoundRule(r, *this, *i, columns, binding);
            //binding.AddRule(b);
            i->AddRule(r);
        }

        // Copy the data across already
        // TODO: Create a rule to transfer the data across.

        struct Adder : public Receiver
        {
            std::shared_ptr<Relation> table;
            void OnRow(Entity *row) override
            {
                table->Add(row);
            }
            Adder(std::shared_ptr<Relation> &t) : table(t) {}
        } adder{i};

        table->ReadAllData(adder);
    }
    return *i;
}

bool Predicate::IsSpecial() const
{
    return false;
}

bool SpecialPredicate::IsSpecial() const
{
    return true;
}

void ExecutionUnit::AddRelation(Relation &rel)
{
    for(auto i : relations)
        if(i == &rel) return;
    
    relations.insert(&rel);
    rel.VisitRules([&](EvaluationPtr &rule)
    {
        // !! Quadratic algorithm - fixme
        for(auto & r : rules.rules)
            if(&*rule == &*r) return;
        rules.rules.push_back(rule);
    });
}

void ExecutionUnit::RunRules()
{
    if (evaluated)
        return;
    evaluated = true;

    if (database.GetVerbosity() > 2)
    {
        std::cout << "About to evaluate:" << std::endl;
        std::cout << rules.rules.size() << " rules:\n";
        Explain();
    }
    
    for (auto r : relations)
    {
        assert(&*r->loop == this);
        r->FirstIteration();
    }

    for (auto &p : rules.rules)
    {
        p->Evaluate(nullptr);
    }

    Size loopSize;

    bool resultsFound;
    do
    {
        for (auto r : relations)
            r->NextIteration();

        loopSize = numberOfResults;

        if (database.GetVerbosity() > 2)
        {
            std::cout << "Number of results in loop = " << loopSize << std::endl;
            Explain();
        }

        for (auto &p : rules.rules)
        {
            if (p->onRecursivePath)
                p->Evaluate(nullptr);
        }
        resultsFound = numberOfResults > loopSize;
        loopSize = numberOfResults;
    } while (resultsFound);

    for (auto r : relations)
        r->NextIteration();

    if (database.Explain())
        Explain();
}

void ExecutionUnit::Explain()
{
    std::cout << "Evaluated ";
    bool first = true;
    for (auto &r : relations)
    {
        if (first)
            first = false;
        else
            std::cout << ", ";
        Evaluation::OutputRelation(std::cout, database, *r);
    }

    std::cout << " ->\n";
    for (auto &p : rules.rules)
    {
        p->Explain(database, std::cout, 4);
    }
}

ExecutionUnit::ExecutionUnit(Database &db) : database(db), rules(db)
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

void RuleSet::Add(const EvaluationPtr &rule)
{
    rules.push_back(rule);
}

void Relation::VisitRules(const std::function<void(Evaluation &)> &fn)
{
    VisitRules([&](EvaluationPtr & rule) { fn(*rule); });
}

void Predicate::VisitRules(const std::function<void(EvaluationPtr &)> &fn)
{
    for (EvaluationPtr &rule : rules.rules)
        fn(rule);
}

RuleSet::RuleSet(Database &db) : rules(db.Storage())
{
}

void Evaluation::VisitSteps(EvaluationPtr &ptr, const std::function<void(EvaluationPtr &)> &fn)
{
    auto ptr2 = ptr;
    fn(ptr);
    ptr2->VisitNext([&](EvaluationPtr &next, bool)
                    { VisitSteps(next, fn); });
}

void RuleSet::VisitRules(const std::function<void(EvaluationPtr &)> &fn)
{
    for (auto &r : rules)
        fn(r);
}

void RuleSet::VisitRules(const std::function<void(Evaluation &)> &fn)
{
    for (auto &r : rules)
        fn(*r);
}

void RuleSet::VisitSteps(const std::function<void(EvaluationPtr &)> &fn)
{
    for (auto &r : rules)
        Evaluation::VisitSteps(r, fn);
}

bool ExecutionUnit::Recursive() const
{
    return recursive;
}

void Predicate::AddExtern(Columns, Logical::Extern, void *)
{
    database.Error("Attempt to redefine predicate as an extern");
}

void Predicate::AddExtern(Logical::Extern, void *)
{
    database.Error("Attempt to redefine predicate as an extern");
}

std::ostream & operator<<(std::ostream & os, const Relation & relation)
{
    Evaluation::OutputRelation(os, relation.GetDatabase(), relation);
    return os;
}

PredicateName::PredicateName() : boundColumns(0) {}

PredicateName::PredicateName(int arity, RelationId object) : arity(arity), boundColumns(0)
{
    objects.parts.push_back(object);
}

void PredicateName::Write(Database & db, std::ostream & os) const
{
    if(objects.parts.empty())
    {
        if(reaches)
            os << "reaches";
        else
            os << "has";
    }
    else
    {
        bool first = true;
        for(auto p : objects.parts)
        {
            if(first) first=false; else os << ";";
            os << db.GetString(p);
        }
    }
    
    if(arity!=1 && attributes.parts.empty())
        os << "/" << arity;
    
    for(auto a : attributes.parts)
    {
        os << "," << db.GetString(a);
    }
    
}

template <typename T>
static void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

int PredicateName::Hash::operator()(const PredicateName& n) const
{
    std::size_t h = 0;
    for(auto n : n.objects.parts)
        hash_combine(h, n);
    for(auto n : n.attributes.parts)
        hash_combine(h, n);
    hash_combine(h, n.reaches);
    hash_combine(h, n.arity);
    return h;
}

bool PredicateName::operator==(const PredicateName & n2) const
{
    return objects==n2.objects &&
        attributes == n2.attributes &&
        reaches==n2.reaches &&
        arity == n2.arity;
}

bool PredicateName::operator!=(const PredicateName & n2) const
{
    return !(*this == n2);
}

bool PredicateName::operator<=(const PredicateName & n2) const
{
    return reaches == n2.reaches && arity <= n2.arity && objects <= n2.objects && attributes <= n2.attributes;
}

bool PredicateName::operator<(const PredicateName & n2) const
{
    return *this <= n2 && *this != n2;
}

SemiNaivePredicate::SemiNaivePredicate(Relation & base, Columns c, const std::shared_ptr<Table> & table) :
    rules(base.GetDatabase()), underlyingPredicate(base), table(table)
{
    name = underlyingPredicate.name;
    name.boundColumns = c;
    
    query = allocate_shared<SemiNaiveQuery>(base.GetDatabase().Storage(), underlyingPredicate, c);
}

void SemiNaivePredicate::Query(Row row, Columns c, Receiver &r)
{
}

void SemiNaivePredicate::QueryDelta(Row row, Columns c, Receiver &r)
{
}

bool SemiNaivePredicate::QueryExists(Row row, Columns c)
{
}

void SemiNaivePredicate::AddRule(const EvaluationPtr & rule)
{
}

void SemiNaivePredicate::RunRules()
{
}

void SemiNaivePredicate::VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &fn)
{
    rules.VisitRules(fn);
}

void SemiNaivePredicate::Add(const Entity * data)
{
}

Size SemiNaivePredicate::Count()
{
}

Database & SemiNaivePredicate::GetDatabase() const
{
}

Relation& SemiNaivePredicate::GetSemiNaivePredicate(Columns columns)
{
}

bool SemiNaivePredicate::IsSpecial() const
{
    return false;
}

void SemiNaivePredicate::FirstIteration()
{
}

void SemiNaivePredicate::NextIteration()
{
}

void SemiNaivePredicate::AddExtern(Columns cols, Logical::Extern ex, void * data)
{
}

void SemiNaivePredicate::AddExtern(Logical::Extern ex, void * data)
{
}

SemiNaiveQuery::SemiNaiveQuery(Relation & base, Columns c)
{
    name = base.name;
    name.boundColumns = c;
    
    name.arity = c.BindCount();
}

void SemiNaiveQuery::Query(Row row, Columns c, Receiver &r) {}
void SemiNaiveQuery::QueryDelta(Row row, Columns c, Receiver &r) {}
bool SemiNaiveQuery::QueryExists(Row row, Columns c) {}
void SemiNaiveQuery::AddRule(const EvaluationPtr & rule) {}
void SemiNaiveQuery::RunRules() {}
void SemiNaiveQuery::VisitRules(const std::function<void(std::shared_ptr<Evaluation>&)> &) {}
void SemiNaiveQuery::Add(const Entity * data) {}
Size SemiNaiveQuery::Count() {}
Database & SemiNaiveQuery::GetDatabase() const {}
Relation& SemiNaiveQuery::GetSemiNaivePredicate(Columns columns) {}
bool SemiNaiveQuery::IsSpecial() const {}
void SemiNaiveQuery::FirstIteration() {}
void SemiNaiveQuery::NextIteration() {}
void SemiNaiveQuery::AddExtern(Columns cols, Logical::Extern ex, void * data) {}
void SemiNaiveQuery::AddExtern(Logical::Extern ex, void * data) {}
