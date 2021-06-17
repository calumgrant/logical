#include "Compiler.hpp"
#include "Database.hpp"
#include "Evaluation.hpp"
#include "EvaluationImpl.hpp"
#include "TableImpl.hpp"
#include "Analysis.hpp"

#include <iostream>


class TableWriterClause : public AST::Clause
{
public:
    TableWriterClause(AST::Clause & lhs) : lhs(lhs)
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
    
    TableWriterClause writer(*lhs);
    rhs->SetNext(writer);

    auto evaluation = rhs->Compile(db, compilation);
    
    evaluation = std::make_shared<RuleEvaluation>(compilation.locals, evaluation);

    
    lhs->AddRule(db, evaluation);
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::AttributeList::Compile(Database & db, Compilation &c, int slot, bool lhsBound, AST::Clause * next, HasType has)
{
    for(auto &a : attributes)
    {
        if(a.entityOpt)
        {
            a.slot = a.entityOpt->BindVariables(db, c, a.bound);
        }
        else
        {
            a.slot = c.AddUnnamedVariable();
            a.bound = false;
        }
    }

    auto eval = next->Compile(db, c);
        
    auto cn = GetCompoundName();
    auto & compoundRelation = has == HasType::reaches ? db.GetReachesRelation(cn.parts[0]) : db.GetRelation(cn);
    
    std::vector<int> inputs(cn.parts.size() + 1), outputs(cn.parts.size()+1);
    if(lhsBound)
        inputs[0] = slot, outputs[0] = -1;
    else
        inputs[0] = -1, outputs[0] = slot;
    
    for(int i=0; i<cn.parts.size(); ++i)
    {
        auto m = 1 + cn.mapFromInputToOutput[i];
        // TODO: If the variable is not used, then the output should be -1.
        if(attributes[i].bound)
            inputs[m] =  attributes[i].slot,
            outputs[m] = -1;
        else
            inputs[m] = -1,
            outputs[m] = attributes[i].slot;
    }
    
    eval = std::make_shared<Join>(compoundRelation, std::move(inputs), std::move(outputs), eval);
    
    for(auto &a : attributes)
    {
        if(a.entityOpt)
            eval = a.entityOpt->Compile(db, c, eval);
    }

    return eval;
}

std::shared_ptr<Evaluation> AST::NotImplementedClause::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::And::Compile(Database &db, Compilation &c)
{
    return lhs->Compile(db, c);
}

std::shared_ptr<Evaluation> AST::EntityClause::Compile(Database &db, Compilation &compilation)
{
    bool bound = false;
    int slot = entity->BindVariables(db, compilation, bound);
    
    if(!bound && is == IsType::isnot)
    {
        // Handle the case `<unbound> is not foo`
        entity->UnboundError(db);
        return std::make_shared<NoneEvaluation>();
    }
    
    assert(next);
    std::shared_ptr<Evaluation> eval;

    if(attributes)
    {
        bool entityBound2;
        entityBound2 = bound || predicates;
    
        // Compile the attributes using the information provided
        eval = attributes->Compile(db, compilation, slot, entityBound2, next, has);
    }
    else
    {
        eval = next->Compile(db, compilation);
    }
    
    /*
     Convert X is not a foo into
        NotInB.
     
     Compile `X is not a foo bar` into an `IsNot`
        `X is foo bar` into `not (X is foo and X is bar)` into `X is not foo or X is not bar`
        
     */

    if(predicates)
    {
        if(is == IsType::is)
        {
            for(int i = predicates->list.size()-1; i>=0; --i)
            {
                auto & relation = db.GetUnaryRelation(predicates->list[i]->nameId);
                if(i>0 || bound)
                    eval = std::make_shared<Join>(relation, std::vector<int> {slot}, std::vector<int>{-1}, eval);
                else
                    eval = std::make_shared<Join>(relation, std::vector<int> {-1}, std::vector<int>{slot}, eval);
            }
        }
        else
        {
            // isnot
            if(predicates->list.size() == 1)
            {
                auto & relation = db.GetUnaryRelation(predicates->list[0]->nameId);
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
    
    explicit NotHandler(const std::shared_ptr<NotTerminator> & terminator) : terminator(terminator)
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

    NotHandler handler(terminator);
    clause->SetNext(handler);
    auto bodyEval = clause->Compile(db, compilation);

    return std::make_shared<Load>(slot, ::Entity(EntityType::Boolean,1),
                                  std::make_shared<OrEvaluationForNot>(bodyEval, std::make_shared<NotNone>(slot, nextEval)));
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::CompileLhs(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::EntityClause::WritePredicates(Database &db, Compilation &c, int slot)
{
    std::shared_ptr<Evaluation> result;
    if(predicates)
    {
        for(auto & i : predicates->list)
        {
            std::shared_ptr<Evaluation> e = std::make_shared<Writer>(db.GetUnaryRelation(i->nameId), std::vector<int> {slot} );
            if(result)
                result = std::make_shared<OrEvaluation>(result, e);
            else
                result = e;
        }
    }
    
    if(attributes)
    {
        auto cn = attributes->GetCompoundName();
        
        auto & relation = db.GetRelation(cn);
        
        std::vector<int> slots(cn.parts.size()+1);
        slots[0] = slot;
        for(int i=0; i<cn.parts.size(); ++i)
        {
            bool bound;
            auto & entity = attributes->attributes[i].entityOpt;
            if(!entity)
            {
                return std::make_shared<NoneEvaluation>();
            }
            slots[cn.mapFromInputToOutput[i]+1] = entity->BindVariables(db, c, bound);
            if(!bound)
            {
                entity->UnboundError(db);
                return std::make_shared<NoneEvaluation>();
            }
        }
        
        std::shared_ptr<Evaluation> e = std::make_shared<Writer>(relation, slots);

        if(result)
            result = std::make_shared<OrEvaluation>(result, e);
        else
            result = e;

        
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
    // TODO
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
    if(predicates)
    {
        for(auto &i : predicates->list)
            db.GetUnaryRelation(i->nameId).AddRule(rule);
    }
    
    if(attributes)
    {
        auto & relation = db.GetRelation(attributes->GetCompoundName());
        relation.AddRule(rule);
        if(predicates)
        {
            for(auto &i : predicates->list)
                db.GetUnaryRelation(i->nameId).AddAttribute(relation);
        }
    }
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
    return std::make_shared<Load>(slot, value, next);
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
        os << "Print row\n";
    }
    
    void VisitVariables(const std::function<void(int&, VariableAccess)> &fn) override
    {
        for(int i=0; i<rowSize; ++i)
            fn(i, VariableAccess::Read);
    }
    
    std::size_t Count() const { return count; }
    
    EvaluationPtr Clone() const override { return nullptr; }

private:
    Database & database;
    const int rowSize;
    std::size_t count;
};

class ResultsPrinter : public AST::Clause
{
public:
    ResultsPrinter(Database &db) : database(db)
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
    ResultsPrinter p(db);
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
    if(!bound1) rhs->UnboundError(db);

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

std::shared_ptr<Evaluation> AST::NegateEntity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    return std::make_shared<NegateBF>(slot1, resultSlot, next);
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
    
    return std::make_shared<Load>(slot, db.CreateInt(0), std::make_shared<OrEvaluation>(eval, next));
};

int AST::Aggregate::BindVariables(Database & db, Compilation &c, bool & bound)
{
    bound = true;
    return slot = c.AddUnnamedVariable();
}

AST::Clause * AST::MakeAll(Clause * ifPart, Clause * thenPart)
{
    return new AST::Not(new AST::And(ifPart, new AST::Not(thenPart)));
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
    
    return std::make_shared<Load>(slot, db.CreateInt(0), std::make_shared<OrEvaluation>(eval, next));
};
