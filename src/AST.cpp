#include "AST.hpp"
#include "Database.hpp"
#include <iostream>

AST::Node::~Node()
{
}

AST::NamedVariable::NamedVariable(const char * name, int line, int column) : Variable(line, column), name(name)
{
}

AST::String::String(const std::string &p) : value(p) 
{ 
}

AST::UnnamedVariable::UnnamedVariable(int line, int column) : Variable(line, column)
{
}

AST::Variable::Variable(int line, int column) : line(line), column(column)
{
}

AST::EntityClause::EntityClause(Entity* entity, UnaryPredicateList* list, UnaryPredicateList * isList, AttributeList * attributes)
    : entity(entity), predicates(list), isPredicates(isList), attributes(attributes)
{
}

AST::EntityIs::EntityIs(Entity * entity, UnaryPredicateList * list) : EntityClause(entity, list)
{
}

AST::EntityIsPredicate::EntityIsPredicate(Entity* entity, UnaryPredicateList* list, UnaryPredicateList *p) :
    EntityClause(entity, list,p)
{
}


AST::Predicate::Predicate(const char * name) : name(name) { }

AST::UnaryPredicate::UnaryPredicate(const char * name) : name(name) { }

AST::BinaryPredicate::BinaryPredicate(const char * name) : Predicate(name) { }

AST::UnaryPredicateList::UnaryPredicateList(UnaryPredicate * p)
{
    Append(p);
}

void AST::UnaryPredicateList::Append(UnaryPredicate * p)
{
    list.push_back(std::unique_ptr<UnaryPredicate>(p));
}

AST::Bool::Bool(bool b) : value(b)
{    
}

AST::Integer::Integer(int v) : value(v)
{
}

AST::Float::Float(double d) : value(d)
{
}

AST::AtString::AtString(const char * v) : value(v)
{   
}

void AST::EntityClause::AssertFacts(Database &db) const
{
    auto v = entity->IsValue();
    if(v)
    {
        ::Entity e(v->MakeEntity(db));
        if(predicates)
            predicates->Assert(db, e);  // Bug: These should be on the right hand side?
        if(isPredicates)
            isPredicates->Assert(db, e);
        
        if(attributes)
            attributes->Assert(db, e);
    }
    else
        entity->UnboundError(db);
}


AST::NotImplementedClause::NotImplementedClause(Node *a, Node *b, Node *c, Node *d)
{
    delete a;
    delete b;
    delete c;
    delete d;
}

void AST::NotImplementedClause::AssertFacts(Database & db) const
{
    std::cerr << "Not implemented.\n";
}

void AST::UnaryPredicate::Assert(Database &db, const ::Entity &e) const
{
    db.GetUnaryRelation(name)->Add(&e);
}

void AST::UnaryPredicateList::Assert(Database &db, const ::Entity &e) const
{
    for(auto &l : list)
        l->Assert(db, e);
}

const AST::Variable * AST::Value::IsVariable() const { return nullptr; }

const AST::Value * AST::Value::IsValue() const { return this; }


Entity AST::Bool::MakeEntity(Database &db) const
{
    return db.Create(value);
}

Entity AST::Float::MakeEntity(Database &db) const
{
    return db.CreateFloat(value);
}

Entity AST::String::MakeEntity(Database &db) const
{
    return db.CreateString(value);
}

Entity AST::Integer::MakeEntity(Database &db) const
{
    return db.CreateInt(value);
}

Entity AST::AtString::MakeEntity(Database &db) const
{
    return db.CreateAt(value);
}

const AST::Variable * AST::Variable::IsVariable() const { return this; }

AST::And::And(Clause *lhs, Clause *rhs) : lhs(lhs), rhs(rhs)
{
}

void AST::And::AssertFacts(Database &db) const
{
    lhs->AssertFacts(db);
    rhs->AssertFacts(db);
}

AST::AttributeList::AttributeList(BinaryPredicate * predicate, Entity * entityOpt, AttributeList *listOpt) :
    predicate(predicate), entityOpt(entityOpt), listOpt(listOpt)
{
}

AST::EntityHasAttributes::EntityHasAttributes(UnaryPredicateList * unaryPredicatesOpt, Entity * entity, AttributeList * attributes) :
    EntityClause(entity, unaryPredicatesOpt, nullptr, attributes)
{
}

void AST::AttributeList::Assert(Database &db, const ::Entity &e) const
{
    if(entityOpt)
    {
        auto v = entityOpt->IsValue();
        if(!v)
        {
            entityOpt->UnboundError(db);
            return;
        }
        ::Entity row[2] = { e, v->MakeEntity(db) };
        auto table = db.GetBinaryRelation(predicate->name);

        table->Add(row);
    }

    if(listOpt) listOpt->Assert(db, e);
}

AST::EntityList::EntityList(Entity *e)
{
    Add(e);
}

void AST::EntityList::Add(Entity *e)
{
    entities.push_back(std::unique_ptr<Entity>(e));
}

AST::DatalogPredicate::DatalogPredicate(Predicate * predicate, EntityList * entityListOpt) :
    predicate(predicate), entitiesOpt(entityListOpt)
{
}

void AST::DatalogPredicate::AssertFacts(Database &db) const
{
    int arity;

    arity = entitiesOpt ? entitiesOpt->entities.size() : 0;

    switch(arity)
    {
    case 1:
        {
            auto v = entitiesOpt->entities[0]->IsValue();
            if(v)
            {
                ::Entity e = v->MakeEntity(db);
                db.GetUnaryRelation(predicate->name)->Add(&e);
            }
            else
                entitiesOpt->entities[0]->UnboundError(db);
            return;
        }
    case 2:
        {
            auto v0 = entitiesOpt->entities[0]->IsValue();
            auto v1 = entitiesOpt->entities[1]->IsValue();
            if(!v0) entitiesOpt->entities[0]->UnboundError(db);
            if(!v1) entitiesOpt->entities[1]->UnboundError(db);

            if(v0 && v1)
            {
                ::Entity row[2] = { v0->MakeEntity(db), v1->MakeEntity(db) };
                db.GetBinaryRelation(predicate->name)->Add(row);
            }

            return;
        }
    }

    auto relation = db.GetRelation(predicate->name, arity);

    // TODO: Add the data 
    std::cout << "TODO: Assert Datalog predicate " << predicate->name << "/" << arity << ".\n";
}

AST::NotImplementedEntity::NotImplementedEntity(AST::Node *n1, AST::Node *n2)
{
    std::unique_ptr<Node> node1(n1), node2(n2);
}

const AST::Variable * AST::NotImplementedEntity::IsVariable() const
{
    return nullptr;
}
    
AST::Rule::Rule(AST::Clause * lhs, AST::Clause * rhs) :
    lhs(lhs), rhs(rhs)
{
}

void AST::NamedVariable::Visit(Visitor&v) const
{
    v.OnVariable(name);
}

void AST::UnaryPredicate::Visit(Visitor&v) const
{
    v.OnUnaryPredicate(name);
}

void AST::UnnamedVariable::Visit(Visitor&v) const
{
    v.OnUnnamedVariable();
}

void AST::DatalogPredicate::Visit(Visitor&v) const
{
    predicate->Visit(v);
    if(entitiesOpt) entitiesOpt->Visit(v);
}

void AST::Visitor::OnVariable(const std::string&)
{
}

void AST::Visitor::OnUnaryPredicate(const std::string&)
{
}

void AST::Visitor::OnUnnamedVariable()
{
}

void AST::EntityClause::Visit(Visitor&v) const
{
    if(entity) entity->Visit(v);
    if(predicates) predicates->Visit(v);
    if(isPredicates) isPredicates->Visit(v);
    if(attributes) attributes->Visit(v);
}

void AST::UnaryPredicateList::Visit(Visitor&v) const
{
    for(auto &i : list)
        i->Visit(v);
}

void AST::NotImplementedClause::Visit(Visitor&) const
{
}

void AST::And::Visit(Visitor&v) const
{
    lhs->Visit(v);
    rhs->Visit(v);
}

void AST::Bool::Visit(Visitor&v) const
{
    v.OnBool(value);
}

void AST::String::Visit(Visitor&v) const
{
    v.OnString(value);
}

void AST::AtString::Visit(Visitor&v) const
{
    v.OnAtString(value);
}

void AST::Float::Visit(Visitor&v) const
{
    v.OnFloat(value);
}

void AST::Integer::Visit(Visitor&v) const
{
    v.OnInteger(value);
}

void AST::NotImplementedEntity::Visit(Visitor&v) const
{
}

void AST::EntityList::Visit(Visitor&v) const
{
    for(auto &e : entities)
        e->Visit(v);
}

void AST::Visitor::OnFloat(double value)
{
}

void AST::Visitor::OnString(const std::string&)
{
}

void AST::Visitor::OnAtString(const std::string&)
{
}

void AST::Visitor::OnBool(bool)
{
}

void AST::Visitor::OnInteger(int)
{
}

void AST::Predicate::Visit(Visitor&v) const
{
    v.OnPredicate(name);
}

void AST::AttributeList::Visit(Visitor &v) const
{
    if(listOpt) listOpt->Visit(v);
    predicate->Visit(v);
    if(entityOpt) entityOpt->Visit(v);
}

void AST::Visitor::OnPredicate(const std::string &name)
{
}

void AST::BinaryPredicate::Visit(Visitor&v) const
{
    v.OnBinaryPredicate(name);
}

void AST::Visitor::OnBinaryPredicate(const std::string &name)
{
}

void AST::Rule::Visit(Visitor&v) const
{
    lhs->Visit(v);
    rhs->Visit(v);
}

AST::Clause::Clause() : next(nullptr)
{
}

void AST::Clause::SetNext(Clause & n)
{
    next = &n;
}

AST::Or::Or(Clause * l, Clause *r) : lhs(l), rhs(r)
{
}

AST::Not::Not(Clause *c) : clause(c)
{
}

void AST::And::SetNext(Clause &n)
{
    lhs->SetNext(*rhs);
    rhs->SetNext(n);
}

const AST::NamedVariable * AST::NamedVariable::IsNamedVariable() const { return this; }
const AST::Value * AST::Variable::IsValue() const { return nullptr; }

const AST::NamedVariable * AST::UnnamedVariable::IsNamedVariable() const { return nullptr; }

const AST::Value * AST::NotImplementedEntity::IsValue() const { return nullptr; }

void AST::Or::AssertFacts(Database &db) const
{
}

void AST::Or::Visit(Visitor&v) const 
{
    lhs->Visit(v);
    rhs->Visit(v);
}

void AST::Or::SetNext(Clause &n)
{
    lhs->SetNext(n);
    rhs->SetNext(n);
}


void AST::Not::AssertFacts(Database &db) const
{
    // TODO: Error
}

void AST::Not::Visit(Visitor&v) const
{
    clause->Visit(v);
}

void AST::Not::SetNext(Clause &c)
{
    next = &c;
    clause->SetNext(*this);
}

AST::Comparator::Comparator(Entity * lhs, ComparatorType type, Entity * rhs) : lhs(lhs), type(type), rhs(rhs)
{
}

void AST::Comparator::AssertFacts(Database &db) const
{
    db.InvalidLhs();
}

void AST::Comparator::Visit(Visitor&visitor) const
{
    lhs->Visit(visitor);
    rhs->Visit(visitor);
}

AST::Range::Range(Entity * lb, ComparatorType cmp1, Entity *e, ComparatorType cmp2, Entity * ub) :
    lowerBound(lb), cmp1(cmp1), entity(e), cmp2(cmp2), upperBound(ub)
{
}

void AST::Range::AssertFacts(Database &db) const
{
    db.InvalidLhs();
}

void AST::Range::Visit(Visitor&v) const
{
    lowerBound->Visit(v);
    entity->Visit(v);
    upperBound->Visit(v);
}

void AST::Range::AddRule(Database &db, const std::shared_ptr<Evaluation>&)
{
    db.InvalidLhs();
}

std::ostream & operator<<(std::ostream & os, ComparatorType t)
{
    switch(t)
    {
    case ComparatorType::eq:
        os << "=";
        break;
    case ComparatorType::neq:
        os << "!=";
        break;
    case ComparatorType::lt:
        os << "<";
        break;
    case ComparatorType::gt:
        os << ">";
        break;
    case ComparatorType::lteq:
        os << "<=";
        break;
    case ComparatorType::gteq:
        os << ">=";
        break;
    }
    return os;
}

AST::NegateEntity::NegateEntity(Entity * e) : entity(e)
{
}

AST::BinaryArithmeticEntity::BinaryArithmeticEntity(Entity *l, Entity *r) : lhs(l), rhs(r)
{
}

AST::AddEntity::AddEntity(Entity *l, Entity *r) : BinaryArithmeticEntity(l, r)
{
}

AST::SubEntity::SubEntity(Entity *l, Entity *r) : BinaryArithmeticEntity(l, r)
{
}

AST::MulEntity::MulEntity(Entity *l, Entity *r) : BinaryArithmeticEntity(l, r)
{
}

AST::DivEntity::DivEntity(Entity *l, Entity *r) : BinaryArithmeticEntity(l, r)
{
}

AST::ModEntity::ModEntity(Entity *l, Entity *r) : BinaryArithmeticEntity(l, r)
{
}

const AST::Value * AST::ArithmeticEntity::IsValue() const { return nullptr; }

const AST::Variable * AST::ArithmeticEntity::IsVariable() const { return nullptr; }

void AST::BinaryArithmeticEntity::Visit(Visitor&v) const
{
    lhs->Visit(v);
    rhs->Visit(v);
}
 
const AST::Variable * AST::NegateEntity::IsVariable() const { return nullptr; }
const AST::Value * AST::NegateEntity::IsValue() const { return nullptr; }

void AST::NegateEntity::Visit(Visitor &v) const
{
    entity->Visit(v);
}

AST::Count::Count(Entity *e, Clause *c) : Aggregate(e, nullptr, c)
{
}

void AST::Aggregate::Visit(Visitor &v) const
{
    entity->Visit(v);
    clause->Visit(v);
}

AST::Aggregate::Aggregate(Entity *e, Entity * v, Clause *c) : entity(e), value(v), clause(c)
{
}

void AST::NamedVariable::UnboundError(Database &db) const
{
    db.UnboundError(name, line, column);
}

void AST::UnnamedVariable::UnboundError(Database &db) const
{
    db.UnboundError("_", line, column);
}

void AST::ArithmeticEntity::UnboundError(Database &db) const
{
    db.UnboundError("arithmetic_expression", 0, 0);
}

void AST::Value::UnboundError(Database &db) const
{
    db.UnboundError("??", 0, 0);  // Shouldn't really get here
}

AST::Sum::Sum(Entity * entity, Entity * value, Clause * clause) : Aggregate(entity, value, clause)
{
}
