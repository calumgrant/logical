#include "Compiler.hpp"
#include "Database.hpp"
#include "Evaluation.hpp"

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
    
    evaluation = std::make_shared<RuleEvaluation>(std::move(compilation.row), evaluation);
    
    if(db.Explain())
    {
        evaluation->Explain(db, std::cout, 0);
    }
    
    lhs->AddRule(db, evaluation);
}

std::shared_ptr<Evaluation> AST::DatalogPredicate::Compile(Database &db, Compilation &c)
{
    return std::make_shared<NoneEvaluation>();
}

std::shared_ptr<Evaluation> AST::AttributeList::Compile(Database & db, Compilation &c, int slot, bool alreadyBound, AST::Clause * next)
{
    bool bound = false;
    int slot2 = entityOpt ? entityOpt->BindVariables(db, c, bound) : c.AddUnnamedVariable();
    
    auto eval = listOpt ? listOpt->Compile(db, c, slot, true, next) : next->Compile(db, c);
    
    auto relation = db.GetBinaryRelation(predicate->name);
        
    if(entityOpt)
    {
        eval = entityOpt->Compile(db, c, eval);
    }

    if(alreadyBound && bound)
    {
        eval = std::make_shared<EvaluateBB>(relation, slot, slot2, eval);
    }
    else if(alreadyBound && !bound)
    {
        eval = std::make_shared<EvaluateBF>(relation, slot, slot2, eval);
    }
    else if(!alreadyBound && bound)
    {
        eval = std::make_shared<EvaluateFB>(relation, slot, slot2, eval);
    }
    else
    {
        eval = std::make_shared<EvaluateFF>(relation, slot, slot2, eval);
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
    /*
        Two cases:
        1) It's bound, either as a variable, or by a value
        2) It's unbound, by a named or an unnamed variable
    */
    bool bound = false;
    int slot = entity->BindVariables(db, compilation, bound);
    
    assert(next);
    std::shared_ptr<Evaluation> eval;

    if(attributes)
    {
        bool entityBound2;
        entityBound2 = bound || predicates;
    
        // Compile the attributes using the information provided
        eval = attributes->Compile(db, compilation, slot, entityBound2, next);
    }
    else
    {
        eval = next->Compile(db, compilation);
    }

    if(predicates)
    {
        for(int i = predicates->list.size()-1; i>=0; --i)
        {
            auto relation = db.GetUnaryRelation(predicates->list[i]->name);
            if(i>0 || bound)
                eval = std::make_shared<EvaluateB>(relation, slot, eval);
            else
                eval = std::make_shared<EvaluateF>(relation, slot, eval);
        }
    }

    return eval;
}

Compilation::Compilation()
{
}

Compilation::~Compilation()
{
}

int Compilation::AddVariable(const std::string &name, bool &alreadybound)
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
        auto size = row.size();
        row.push_back(Entity());
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
    auto size = row.size();
    row.push_back(Entity());
    return size;
}

int Compilation::AddValue(const Entity &e)
{
    auto size = row.size();
    row.push_back(e);
    return size;
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
    int branch = compilation.CreateBranch();

    auto terminator = std::make_shared<NotTerminator>();
    
    compilation.Branch(branch);
    auto nextEval = next->Compile(db, compilation);
    compilation.Branch(branch);

    NotHandler handler(terminator);
    clause->SetNext(handler);
    auto bodyEval = clause->Compile(db, compilation);

    return std::make_shared<NotEvaluation>(terminator, bodyEval, nextEval);
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
            auto e = std::make_shared<WriterB>(db.GetUnaryRelation(i->name), slot);
            if(result)
                result = std::make_shared<OrEvaluation>(result, e);
            else
                result = e;
        }
    }
    
    if(attributes)
    {
        for(auto i = &attributes; *i; i = &(*i)->listOpt)
        {
            bool bound;
            int slot2 = (*i)->entityOpt->BindVariables(db, c, bound);
            if(bound)
            {
                std::shared_ptr<Evaluation> e = std::make_shared<WriterBB>(db.GetBinaryRelation((*i)->predicate->name), slot, slot2);
                e = (*i)->entityOpt->Compile(db, c, e);
                if(result)
                    result = std::make_shared<OrEvaluation>(result, e);
                else
                    result = e;
            }
            else
                (*i)->entityOpt->UnboundError(db);
        }
    }

    if(!result) result = std::make_shared<NoneEvaluation>();
    return result;
}

std::shared_ptr<Evaluation> AST::EntityClause::CompileLhs(Database &db, Compilation &c)
{
    bool bound;
    int slot = entity->BindVariables(db, c, bound);
    if(bound)
    {
        return WritePredicates(db, c, slot);
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

void AST::And::AddRule(Database &db, const std::shared_ptr<Evaluation> & rule)
{
    // ??
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
            db.GetUnaryRelation(i->name)->AddRule(rule);
    }
    
    for(auto p = &attributes; *p; p=&(*p)->listOpt)
    {
        db.GetBinaryRelation((*p)->predicate->name)->AddRule(rule);
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
            db.UnboundError("="); // !! Better error message
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
            db.UnboundError("="); // !! Better error message
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
    return c.AddVariable(name, bound);
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
    return c.AddValue(MakeEntity(db));
}

class ResultsPrinterEval : public Evaluation
{
public:
    ResultsPrinterEval(Database & db, int rs) : database(db), rowSize(rs), count() { }
    
    
    void Evaluate(Entity * row) override
    {
        std::cout << "\t";
        for(int i=0; i<rowSize; ++i)
        {
            if(i>0) std::cout << ", ";
            database.PrintQuoted(row[i], std::cout);
        }
        std::cout << std::endl;
        ++count;
    }
    
    void Explain(Database &db, std::ostream &os, int indent) const override
    {
        Indent(os, indent);
        os << "Print row\n";
    }
    
    std::size_t Count() const { return count; }
    
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
        return printer = std::make_shared<ResultsPrinterEval>(db, compilation.row.size());
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

    eval->Evaluate(&c.row[0]);
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

    auto nextEval = next->Compile(db, compilation);

    if(bound3)
    {
        return std::make_shared<RangeB>(slot1, cmp1, slot3, cmp2, slot2, nextEval);
    }
    else
    {
        return std::make_shared<RangeU>(slot1, cmp1, slot3, cmp2, slot2, nextEval);
    }
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

std::shared_ptr<Evaluation> AST::SubEntity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    return std::make_shared<SubBBF>(lhsSlot, rhsSlot, resultSlot, next);
}

std::shared_ptr<Evaluation> AST::MulEntity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    // TODO: Repeat a string by multiplying it.
    return std::make_shared<MulBBF>(lhsSlot, rhsSlot, resultSlot, next);
}

std::shared_ptr<Evaluation> AST::DivEntity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    return std::make_shared<DivBBF>(lhsSlot, rhsSlot, resultSlot, next);
}

std::shared_ptr<Evaluation> AST::ModEntity::Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const
{
    return std::make_shared<ModBBF>(lhsSlot, rhsSlot, resultSlot, next);
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
            result = std::make_shared<DeduplicateBB>(slot, compilation.AddUnnamedVariable(), next);
        return result;
    }

private:
    AST::Entity & entity;
    const std::shared_ptr<CountCollector> next;
    
    std::shared_ptr<Evaluation> result;
};

std::shared_ptr<Evaluation> AST::Aggregate::Compile(Database &db, Compilation&c, const std::shared_ptr<Evaluation> & next) const
{
    auto collector = std::make_shared<CountCollector>();
    
    int slot2 = c.AddUnnamedVariable();

    bool bound;
    CountTerminatorClause terminator(*entity, collector);

    clause->SetNext(terminator);
    
    int branch = c.CreateBranch();
    auto eval = clause->Compile(db, c);
    
    //bool bound2;
    // !! This should happen in the terminator
    //int slot3 = entity->BindVariables(db, c, bound2); // Do we care about this??
    //if(!bound)
     //   db.UnboundError("...");
    
    c.Branch(branch);

    std::shared_ptr<Evaluation> tail = std::make_shared<CountEvaluation>(slot, collector, next);
    
    return std::make_shared<OrEvaluation>(eval, tail);
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
