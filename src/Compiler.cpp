#include "Compiler.hpp"
#include "Database.hpp"
#include "Evaluation.hpp"
#include "EvaluationImpl.hpp"
#include "TableImpl.hpp"
#include "Analysis.hpp"

#include <iostream>
#include <cassert>

class TableWriterClause : public AST::Clause
{
public:
    TableWriterClause(const SourceLocation & loc, AST::Clause & lhs) : AST::Clause(loc), lhs(lhs)
    {
    }
    
    void Visit(AST::Visitor &v) const override { }
    void AssertFacts(Database &db) const override {}
        
    std::shared_ptr<Evaluation> Compile(Database &db, Compilation &c) override
    {
        return lhs.CompileLhs(db, c);
    }
    
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override
    {
        assert(!"Impossible");
        return std::make_shared<NoneEvaluation>();
    }

    void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override
    {
    }

    
private:
    AST::Clause & lhs;
};

void AST::Rule::Compile(Database &db)
{
    Compilation compilation;
    
    TableWriterClause writer(lhs->location, *lhs);
    rhs->SetNext(writer);

    try
    {
        auto evaluation = rhs->Compile(db, compilation);
        
        evaluation = std::make_shared<RuleEvaluation>(compilation.locals, evaluation);
        
        lhs->AddRule(db, evaluation);
    }
    catch(CompilationError &error)
    {
        error.ReportError(db);
    }
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::Compile(Database &db, Compilation &c)
{
    auto name = GetPredicateName(db);
    std::vector<bool> boundVars(name.arity);
    std::vector<int> inputs(name.arity), outputs(name.arity);
    
    if(entitiesOpt)
    {
        for(int i=0; i<name.arity; ++i)
        {
            bool bound = false;
            auto var = entitiesOpt->entities[i]->BindVariables(db, c, bound);
            boundVars[i] = bound;
            auto j = name.MapArgument(i);
            if(bound) { inputs[j] = var, outputs[j] = -1; }
            else { inputs[j] = -1, outputs[j] = var; }
        }
    }

    auto result = next->Compile(db, c);
    
    if(entitiesOpt)
    {
        for(int i=0; i<name.arity; ++i)
        {
            for(int j=0; j<i; ++j)
            {
                if(outputs[j] == inputs[i] && inputs[i] != -1)
                {
                    outputs[i] = c.AddUnnamedVariable();
                    inputs[i] = -1;
                    result = std::make_shared<EqualsBB>(outputs[j], outputs[i], result);
                    break;
                }
            }
        }
    }

        
    auto & relation = db.GetRelation(name);
    result = std::make_shared<Join>(relation, inputs, outputs, result, location);

    if(entitiesOpt)
    {
        for(auto & i : entitiesOpt->entities)
            result = i->Compile(db, c, result);
    }
    return result;
}

std::shared_ptr<Evaluation> AST::NotImplementedClause::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::And::Compile(Database &db, Compilation &c)
{
    return lhs->Compile(db, c);
}

bool AST::EntityClause::Decompose(const Database & db) const
{
    bool decompose = has != HasType::comma;
    
    if(predicates && db.IsExtern(predicates->list[0]->nameId))
        decompose = false;
    
    if(is == IsType::isnot) return false;

    return decompose;
}

EvaluationPtr AST::EntityClause::CompileDecomposed(Database &db, Compilation &compilation, int slot, bool lhsBound)
{
    auto eval = next->Compile(db, compilation);
    
    if(attributes)
    {
        for(int i=attributes->attributes.size()-1; i>=0; --i)
        {
            auto & attribute = attributes->attributes[i];
            PredicateName attributeName;
            attributeName.arity = 2;
            attributeName.attributes.parts.push_back(attribute.predicates[0]->nameId);
            
            if(has == HasType::reaches)
                attributeName.reaches = true;
            
            auto & relation = db.GetRelation(attributeName);
            
            std::vector<int> inputs(2), outputs(2);
            if(i>0 || predicates || lhsBound)
            {
                inputs[0] = slot;
                outputs[0] = -1;
            }
            else
            {
                inputs[0] = -1;
                outputs[0] = slot;
            }
            
            int slot2 = attributes->attributes[i].slot;
            
            if(attribute.bound)
            {
                inputs[1] = slot2;
                outputs[1] = -1;
            }
            else
            {
                inputs[1] = -1;
                outputs[1] = slot2;
            }
            
            eval = std::make_shared<Join>(relation, inputs, outputs, eval, attribute.location);

            if(attribute.entityOpt)
            {
                eval = attribute.entityOpt->Compile(db, compilation, eval);
            }
        }
    }
    
    if(predicates)
    {
        for(int i=predicates->list.size()-1; i>=0; --i)
        {
            PredicateName predicateName;
            predicateName.arity = 1;
            auto & predicate = predicates->list[i];
            predicateName.objects.parts.push_back(predicate->nameId);
            auto & relation = db.GetRelation(predicateName);
            
            std::vector<int> inputs(1), outputs(1);
            if(i==0 && !lhsBound)
            {
                inputs[0] = -1;
                outputs[0] = slot;
            }
            else
            {
                inputs[0] = slot;
                outputs[0] = -1;
            }
            eval = std::make_shared<Join>(relation, inputs, outputs, eval, predicate->location);
        }
    }
    
    if(entity)
    {
        eval = entity->Compile(db, compilation, eval);
    }
    
    return eval;
}

std::shared_ptr<Evaluation> AST::EntityClause::Compile(Database &db, Compilation &compilation)
{
    bool lhsBound = false;
    int slot = entity->BindVariables(db, compilation, lhsBound);
    
    if(!lhsBound && is == IsType::isnot)
    {
        // Handle the case `<unbound> is not foo`
        entity->UnboundError(db);
        return std::make_shared<NoneEvaluation>();
    }

    if(attributes)
    {
        for(auto &a : attributes->attributes)
        {
            if(a.entityOpt)
            {
                a.slot = a.entityOpt->BindVariables(db, compilation, a.bound);
            }
            else
            {
                a.slot = compilation.AddUnnamedVariable();
                a.bound = false;
            }
        }
    }
    
    if(Decompose(db))
    {
        return CompileDecomposed(db, compilation, slot, lhsBound);
    }

    PredicateName name = GetPredicateName();
    
    auto & relation = db.GetRelation(name);
    
    assert(next);
    auto eval = next->Compile(db, compilation);

    std::vector<int> inputs(name.arity), outputs(name.arity);
    if(lhsBound)
        inputs[0] = slot, outputs[0] = -1;
    else
        inputs[0] = -1, outputs[0] = slot;
    
    if(attributes)
    {
        for(int i=0; i<name.attributes.parts.size(); ++i)
        {
            auto m = 1 + name.attributes.mapFromInputToOutput[i];
            if(attributes->attributes[i].bound)
                inputs[m] =  attributes->attributes[i].slot,
                outputs[m] = -1;
            else
                inputs[m] = -1,
                outputs[m] = attributes->attributes[i].slot;
        }
    }
        
    eval = std::make_shared<Join>(relation, std::move(inputs), std::move(outputs), eval, location);
    
    if(attributes)
    {
        for(auto &a : attributes->attributes)
        {
            if(a.entityOpt)
                eval = a.entityOpt->Compile(db, compilation, eval);
        }
    }
    
    /*
     Convert X is not a foo into
        NotInB.
     
     Compile `X is not a foo bar` into an `IsNot`
        `X is foo bar` into `not (X is foo and X is bar)` into `X is not foo or X is not bar`
        
     */

    if(predicates && !attributes)
    {
        if(is == IsType::isnot)
        {
            // isnot
            if(predicates->list.size() == 1)
            {
                PredicateName name(1, predicates->list[0]->nameId);
                auto & relation = db.GetRelation(name);
                eval = std::make_shared<NotInB>(slot, relation, eval);
            }
            else
            {
                // TODO: Use ExistsF.
                std::cout << "Unhandled: is not a foo bar\n";
            }
        }
    }

    eval = entity->Compile(db, compilation, eval);

    return eval;
}

Compilation::Compilation() : locals(0)
{
}

Compilation::~Compilation()
{
}

int Compilation::AddVariable(int name, bool &alreadybound)
{
    auto i = variables.find(name);
    auto j = boundVariables2.find(name);
    
    bool alreadySeen = i != variables.end();
    alreadybound = j != boundVariables2.end();

    if( alreadySeen )
    {
        if(!alreadybound)
        {
            boundVariables2.insert(name);
            boundVariables.push_back(name);
        }
        return i->second;
    }
    else
    {
        auto size = locals++;
        variables[name] = size;
        boundVariables2.insert(name);
        boundVariables.push_back(name);

        return size;
    }
}

int Compilation::CreateBranch()
{
    return boundVariables.size();
}

void Compilation::Branch(int branch)
{
    while(boundVariables.size() > branch)
    {
        auto name = std::move(boundVariables.back());
        boundVariables.pop_back();
        boundVariables2.erase(name);
    }
}

int Compilation::AddUnnamedVariable()
{
    return locals++;
}

int Compilation::AddValue(const Entity &e)
{
    return locals++;
}

std::shared_ptr<Evaluation> AST::Or::Compile(Database &db, Compilation & compilation)
{
    int branch = compilation.CreateBranch();
    auto l = lhs->Compile(db, compilation);
    compilation.Branch(branch);
    auto r = rhs->Compile(db, compilation);

    return std::make_shared<OrEvaluation>(l, r);
}

class NotHandler : public AST::Clause
{
public:
    std::shared_ptr<NotTerminator> terminator;
    
    explicit NotHandler(const SourceLocation & loc, const std::shared_ptr<NotTerminator> & terminator) : AST::Clause(loc), terminator(terminator)
    {
    }
    
    void SetNext(Clause&) override
    {
        
    }
        
    virtual std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override
    {
        return terminator;
    }
        
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override
    {
        return std::make_shared<NoneEvaluation>();
    }
    
    void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override
    {
    }
    
    void Visit(AST::Visitor&) const override {}
    
    void AssertFacts(Database&) const override
    {
    }
};

std::shared_ptr<Evaluation> AST::Not::Compile(Database &db, Compilation & compilation)
{
    int slot = compilation.AddUnnamedVariable();
    
    int branch = compilation.CreateBranch();

    auto terminator = std::make_shared<NotTerminator>(slot);
    
    compilation.Branch(branch);
    auto nextEval = next->Compile(db, compilation);
    compilation.Branch(branch);

    NotHandler handler(location, terminator);
    clause->SetNext(handler);
    auto bodyEval = clause->Compile(db, compilation);

    return std::make_shared<LoadF>(slot, ::Entity(EntityType::Boolean,1),
                                  std::make_shared<OrEvaluationForNot>(bodyEval, std::make_shared<NotNone>(slot, nextEval)));
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::CompileLhs(Database &db, Compilation &c)
{
    auto & relation = db.GetRelation(GetPredicateName(db));

    if(entitiesOpt)
    {
        std::vector<int> row(entitiesOpt->entities.size());

        for(int i=0; i<entitiesOpt->entities.size(); ++i)
        {
            bool bound;
            row[i] = entitiesOpt->entities[i]->BindVariables(db, c, bound);
            if(!bound)
            {
                entitiesOpt->entities[i]->UnboundError(db);
            }
        }
        
        EvaluationPtr result = std::make_shared<Writer>(relation, row, location);
        
        for(auto & i : entitiesOpt->entities)
            result = i->Compile(db, c, result);
        
        return result;
    }
    else
    {
        return std::make_shared<Writer>(relation, std::vector<int>(), location);
    }
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityClause::WritePredicates(Database &db, Compilation &c, int slot)
{
    PredicateName name = GetPredicateName();
    
    auto & relation = db.GetRelation(name);
    
    std::vector<int> slots(name.arity);
    slots[0] = slot;

    if(attributes)
    {
        for(int i=0; i<name.attributes.parts.size(); ++i)
        {
            bool bound;
            auto & entity = attributes->attributes[i].entityOpt;
            if(!entity)
            {
                return std::make_shared<NoneEvaluation>();
            }
            slots[name.attributes.mapFromInputToOutput[i]+1] = entity->BindVariables(db, c, bound);
            if(!bound)
            {
                entity->UnboundError(db);
                return std::make_shared<NoneEvaluation>();
            }
        }
    }

    // The last thing we do is to write the result
    std::shared_ptr<Evaluation> result = std::make_shared<Writer>(relation, slots, location);
    
    if(attributes)
    {
        if(!entity)
        {
            result = std::make_shared<CreateNew>(db, slot, result);
            slots.erase(slots.begin(), slots.begin()+1);  // Ugly and slow
            if(!newEntityTable)
                newEntityTable = std::make_shared<TableImpl>(db.Storage(), slots.size());

            result = std::make_shared<DeduplicateV>(db, slots, newEntityTable, result);
        }
        
        for(auto &a : attributes->attributes)
        {
            result = a.entityOpt->Compile(db, c, result);
        }
    }

    if(!result) result = std::make_shared<NoneEvaluation>();
    return result;
}

PredicateName AST::EntityClause::GetPredicateName() const
{
    PredicateName name;
    
    if(predicates)
    {
        std::vector<int> parts;
        for(auto & i : predicates->list)
            parts.push_back(i->nameId);
        
        name.objects = parts;  // Ensure it's sorted
    }
    
    if(attributes)
    {
        name.attributes = attributes->GetCompoundName();
        name.arity = name.attributes.parts.size()+1;
        if(has == HasType::reaches)
            name.reaches = true;
    }
    else
    {
        name.arity = 1;
    }
    return std::move(name);
}

std::shared_ptr<Evaluation> AST::EntityClause::CompileLhs(Database &db, Compilation &c)
{
    if(!entity)
    {
        int slot = c.AddUnnamedVariable();
        auto eval = WritePredicates(db, c, slot);
        return eval;
    }
    
    bool bound;
    int slot = entity->BindVariables(db, c, bound);
    if(bound)
    {
        auto eval = WritePredicates(db, c, slot);
        eval = entity->Compile(db, c, eval);
        return eval;
    }
    else
    {
        entity->UnboundError(db);
    }

    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::NotImplementedClause::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::And::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<OrEvaluation>(lhs->CompileLhs(db, c), rhs->CompileLhs(db, c));
}

void AST::And::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    lhs->AddRule(db, rule);
    rhs->AddRule(db, rule);
}

std::shared_ptr<Evaluation> AST::Or::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::Not::CompileLhs(Database &db, Compilation &c)
{
    db.InvalidLhs();
    return std::make_shared<NoneEvaluation>();
}

void AST::DatalogPredicate::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // TODO: Cache the predicatename
    auto & relation = db.GetRelation(GetPredicateName(db));
    relation.AddRule(rule);
}

void AST::NotImplementedClause::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::Or::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::Not::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
}

void AST::EntityClause::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    db.GetRelation(GetPredicateName()).AddRule(rule);
}

std::shared_ptr<Evaluation> AST::Comparator::Compile(Database &db, Compilation & compilation)
{
    bool bound1, bound2;
    int slot1 = lhs->BindVariables(db, compilation, bound1);
    int slot2 = rhs->BindVariables(db, compilation, bound2);

    auto eval = next->Compile(db, compilation);

    if(type == ComparatorType::eq)
    {
        if(!bound1 && !bound2)
        {
            lhs->UnboundError(db);
            rhs->UnboundError(db);
            return std::make_shared<NoneEvaluation>();
        }
                
        if(bound1 && bound2)
            eval = std::make_shared<EqualsBB>(slot1, slot2, eval);
        if(bound1 && !bound2)
            eval = std::make_shared<EqualsBF>(slot1, slot2, eval);
        if(bound2 && !bound1)
            eval = std::make_shared<EqualsBF>(slot2, slot1, eval);
    }
    else
    {
        if(!bound1 || !bound2)
        {
            if(!bound1) lhs->UnboundError(db);
            if(!bound2) rhs->UnboundError(db);
            eval = std::make_shared<NoneEvaluation>();
        }

        eval = std::make_shared<CompareBB>(slot1, type, slot2, eval);
    }
    
    eval = rhs->Compile(db, compilation, eval);
    eval = lhs->Compile(db, compilation, eval);
    
    return eval;
}

std::shared_ptr<Evaluation> AST::Comparator::CompileLhs(Database &db, Compilation &compilation)
{
    db.InvalidLhs();
    return std::make_shared<NoneEvaluation>();
}

// void SetNext(Clause&) override;
void AST::Comparator::AddRule(Database &db, const std::shared_ptr<Evaluation>&)
{
    db.InvalidLhs();
}

int AST::NamedVariable::BindVariables(Database &db, Compilation &c, bool &bound)
{
    return c.AddVariable(nameId, bound);
}

int AST::UnnamedVariable::BindVariables(Database &db, Compilation &c, bool &bound)
{
    bound = false;
    return c.AddUnnamedVariable();
}

int AST::NotImplementedEntity::BindVariables(Database &db, Compilation &c, bool &bound)
{
    bound = false;
    return 0;
}

int AST::Value::BindVariables(Database &db, Compilation &c, bool &bound)
{
    bound = true;
    slot = c.AddValue(GetValue());
    return slot;
}

std::shared_ptr<Evaluation> AST::Value::Compile(Database &db, Compilation &c, const std::shared_ptr<Evaluation> & next) const
{
    return std::make_shared<LoadF>(slot, value, next);
}

class ResultsPrinterEval : public Evaluation
{
public:
    ResultsPrinterEval(Database & db, int rs) : database(db), rowSize(rs), count() { }
    
    void OnRow(Entity * row) override
    {
        database.AddResult(row, rowSize, true);
        ++count;
    }
    
    void Explain(Database &db, std::ostream &os, int indent) const override
    {
        Indent(os, indent);
        os << "Print row";
        OutputCallCount(os);
        os << "\n";
    }
    
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override
    {
        for(int i=0; i<rowSize; ++i)
            fn(i, VariableAccess::Read);
    }
    
    std::size_t Count() const { return count; }
    
    EvaluationPtr MakeClone() const override { return nullptr; }
        
private:
    Database & database;
    const int rowSize;
    std::size_t count;
};

class ResultsPrinter : public AST::Clause
{
public:
    ResultsPrinter(const SourceLocation & loc, Database &db) : AST::Clause(loc), database(db)
    {
    }
    
    std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override
    {
        return printer = std::make_shared<ResultsPrinterEval>(db, compilation.locals);
    }
    
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override
    {
        return std::make_shared<NoneEvaluation>();
    }
    
    void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override { }
    
    void Visit(AST::Visitor&) const override { }
    
    void AssertFacts(Database &db) const override { }
    
    std::size_t Count() const { return printer ? printer->Count() : 0; }

private:
    Database & database;
    std::shared_ptr<ResultsPrinterEval> printer;
};

void AST::Clause::Find(Database &db)
{
    Compilation c;
    ResultsPrinter p(location, db);
    SetNext(p);
    auto eval = Compile(db, c);
    
    db.GetOptimizer().Optimize(eval);

    std::vector<::Entity> row(c.locals);
    eval->Evaluate(&row[0]);
    if(db.Explain())
        eval->Explain(db, std::cout);
    
    std::cout << "Found " << p.Count() << " results\n";
}

std::shared_ptr<Evaluation> AST::Range::Compile(Database &db, Compilation & compilation)
{
    bool bound1, bound2, bound3;
    int slot1 = lowerBound->BindVariables(db, compilation, bound1);
    int slot2 = upperBound->BindVariables(db, compilation, bound2);
    int slot3 = entity->BindVariables(db, compilation, bound3);

    if(!bound1 || !bound2)
    {
        if(!bound1) lowerBound->UnboundError(db);
        if(!bound2) upperBound->UnboundError(db);
        return std::make_shared<NoneEvaluation>();
    }

    auto eval = next->Compile(db, compilation);

    if(bound3)
    {
        eval = std::make_shared<RangeB>(slot1, cmp1, slot3, cmp2, slot2, eval);
    }
    else
    {
        eval = std::make_shared<RangeU>(slot1, cmp1, slot3, cmp2, slot2, eval);
    }
    
    eval = entity->Compile(db, compilation, eval);
    eval = upperBound->Compile(db, compilation, eval);
    eval = lowerBound->Compile(db, compilation, eval);
    
    return eval;
}

std::shared_ptr<Evaluation> AST::Range::CompileLhs(Database &db, Compilation &compilation)
{
    db.InvalidLhs();
    return std::make_shared<NoneEvaluation>();
}

int AST::BinaryArithmeticEntity::BindVariables(Database &db, Compilation &c, bool &bound)
{
    bound = true;
    bool bound1, bound2;
    lhsSlot = lhs->BindVariables(db, c, bound1);
    if(!bound1) lhs->UnboundError(db);
    rhsSlot = rhs->BindVariables(db, c, bound2);
    if(!bound2) rhs->UnboundError(db);

    resultSlot = c.AddUnnamedVariable();
    
    return resultSlot;
}


int AST::NegateEntity::BindVariables(Database & db, Compilation &c, bool & bound)
{
    slot1 = entity->BindVariables(db, c, bound);
    
    if(!bound) entity->UnboundError(db);
    bound = true;
    resultSlot = c.AddUnnamedVariable();
    return resultSlot;
}

std::shared_ptr<Evaluation> AST::Entity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    return next;
}

std::shared_ptr<Evaluation> AST::NegateEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto eval = std::make_shared<NegateBF>(slot1, resultSlot, next);
    return entity->Compile(db, c, eval);
}

std::shared_ptr<Evaluation> AST::AddEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto eval = next;
    eval = std::make_shared<AddBBF>(db, lhsSlot, rhsSlot, resultSlot, eval);
    eval = rhs->Compile(db, c, eval);
    eval = lhs->Compile(db, c, eval);
    return eval;
}

std::shared_ptr<Evaluation> AST::SubEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto eval = next;
    eval = std::make_shared<SubBBF>(lhsSlot, rhsSlot, resultSlot, eval);
    eval = rhs->Compile(db, c, eval);
    eval = lhs->Compile(db, c, eval);
    return eval;
}

std::shared_ptr<Evaluation> AST::MulEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    // TODO: Repeat a string by multiplying it.
    auto eval = next;
    eval = std::make_shared<MulBBF>(lhsSlot, rhsSlot, resultSlot, next);
    eval = rhs->Compile(db, c, eval);
    eval = lhs->Compile(db, c, eval);
    return eval;
}

std::shared_ptr<Evaluation> AST::DivEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto eval = next;
    eval = std::make_shared<DivBBF>(lhsSlot, rhsSlot, resultSlot, next);
    eval = rhs->Compile(db, c, eval);
    eval = lhs->Compile(db, c, eval);
    return eval;
}

std::shared_ptr<Evaluation> AST::ModEntity::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto eval = next;
    eval = std::make_shared<ModBBF>(lhsSlot, rhsSlot, resultSlot, next);
    eval = rhs->Compile(db, c, eval);
    eval = lhs->Compile(db, c, eval);
    return eval;
}

class DummyClause : public AST::Clause
{
public:
    DummyClause() : AST::Clause(SourceLocation{}) {}
    
    std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override
    {
        return std::make_shared<NoneEvaluation>();
    }
    
    void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override {}
    
    void Visit(AST::Visitor&) const override {}
    
    void AssertFacts(Database &db) const override {}
};

class CountTerminatorClause : public DummyClause
{
public:
    CountTerminatorClause(AST::Entity & entity, const std::shared_ptr<CountCollector> & next) : entity(entity), next(next) { }
    
    std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override
    {            
        bool bound;
        int slot = entity.BindVariables(db, compilation, bound);
        if(!bound) entity.UnboundError(db);
        
        // Need to share the counter between all branches for example
        // count X in (f X or g X)
        if(!result)
            result = std::make_shared<DeduplicateB>(slot, next);
        return result;
    }
    std::shared_ptr<Deduplicate> result;

private:
    AST::Entity & entity;
    const std::shared_ptr<CountCollector> next;
    
};

std::shared_ptr<Evaluation> AST::Count::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto collector = std::make_shared<CountCollector>(slot);
    
    CountTerminatorClause terminator(*entity, collector);

    clause->SetNext(terminator);
    
    int branch = c.CreateBranch();
    auto eval = clause->Compile(db, c);
    eval = std::make_shared<DeduplicationGuard>(terminator.result, eval);
        
    c.Branch(branch);
    
    return std::make_shared<LoadF>(slot, db.CreateInt(0), std::make_shared<OrEvaluation>(eval, next));
};

int AST::Aggregate::BindVariables(Database & db, Compilation &c, bool & bound)
{
    bound = true;
    return slot = c.AddUnnamedVariable();
}

AST::Clause * AST::MakeAll(Clause * ifPart, Clause * thenPart)
{
    return new AST::Not(ifPart->location + thenPart->location, new AST::And(ifPart->location + thenPart->location, ifPart, new AST::Not(thenPart->location, thenPart)));
}

class SumTerminatorClause : public DummyClause
{
public:
    SumTerminatorClause(int resultSlot, AST::Entity &e, AST::Entity & v) : resultSlot(resultSlot), entity(e), value(v) { }
    
    // Where the result is stored.
    std::shared_ptr<SumCollector> collector;
    const int resultSlot;
    AST::Entity & entity, &value;
    
    std::shared_ptr<Deduplicate> result;
    
    std::shared_ptr<Evaluation> Compile(Database & db, Compilation&c) override
    {
        // Check the variables
        bool bound;
        int entitySlot = entity.BindVariables(db, c, bound);
        
        if(!bound)
            entity.UnboundError(db);
        
        int valueSlot = value.BindVariables(db, c, bound);
        if(!bound)
            value.UnboundError(db);

        if(!result)
        {
            collector = std::make_shared<SumCollector>(valueSlot, resultSlot);
            if (entitySlot == valueSlot)
                result = std::make_shared<DeduplicateB>(entitySlot, collector);
            else
                result = std::make_shared<DeduplicateBB>(entitySlot, valueSlot, collector);
        }
        
        return result;
    }
};

std::shared_ptr<Evaluation> AST::Sum::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    SumTerminatorClause terminator(slot, entity ? *entity : *value, *value);

    clause->SetNext(terminator);
    
    int branch = c.CreateBranch();
    auto eval = clause->Compile(db, c);
        
    c.Branch(branch);
    eval = std::make_shared<DeduplicationGuard>(terminator.result, eval);
    
    return std::make_shared<LoadF>(slot, db.CreateInt(0), std::make_shared<OrEvaluation>(eval, next));
};
