#include "AST.hpp"
#include "Database.hpp"
#include <iostream>

AST::Node::~Node()
{
}

AST::NamedVariable::NamedVariable(const char * name) : name(name)
{
}

AST::String::String(const std::string &p) : value(p) 
{ 
}

AST::TermIs::TermIs(Entity* entity, UnaryPredicateOrList* list)
    : entity(entity), list(list)
{
}

AST::TermIsPredicate::TermIsPredicate(Entity* entity, UnaryPredicateOrList* list, UnaryPredicate *p)
    : entity(entity), list(list), predicate(p)
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

void AST::TermIs::AssertFacts(Database &db) const
{
    list->Assert(db, entity->MakeEntity(db));
}

void AST::TermIsPredicate::AssertFacts(Database &db) const
{
    ::Entity e(entity->MakeEntity(db));
    list->Assert(db, e);
    predicate->Assert(db, e);
}


AST::NotImplementedTerm::NotImplementedTerm(Node *a, Node *b, Node *c, Node *d)
{
    delete a;
    delete b;
    delete c;
    delete d;
}

void AST::NotImplementedTerm::AssertFacts(Database & db) const
{
    std::cerr << "Not implemented.\n";
}

void AST::UnaryPredicate::Assert(Database &db, const ::Entity &e) const
{
    db.GetUnaryRelation(name).Add(e);
}

void AST::UnaryPredicateList::Assert(Database &db, const ::Entity &e) const
{
    for(auto &l : list)
        l->Assert(db, e);
}

bool AST::Value::IsVariable() const { return false; }

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

bool AST::Variable::IsVariable() const { return true; }

Entity AST::NamedVariable::MakeEntity(Database &db) const
{
    db.UnboundError(name);
    return db.CreateInt(-1);
}

Entity AST::UnnamedVariable::MakeEntity(Database &db) const
{
    db.UnboundError("_");
    return db.CreateInt(-1);
}

AST::And::And(Term *lhs, Term *rhs) : lhs(lhs), rhs(rhs)
{
}

void AST::And::AssertFacts(Database &db) const
{
    lhs->AssertFacts(db);
    rhs->AssertFacts(db);
}

AST::AttributeList::AttributeList(BinaryPredicate * predicate, Entity * entityOpt, AttributeList *list) :
    predicate(predicate), entityOpt(entityOpt), list(list)
{
}

AST::EntityHasAttributes::EntityHasAttributes(UnaryPredicateOrList * unaryPredicatesOpt, Entity * entity, AttributeList * attributes) :
    unaryPredicatesOpt(unaryPredicatesOpt), entity(entity), attributes(attributes)
{
}

void AST::EntityHasAttributes::AssertFacts(Database &db) const
{
    ::Entity e = entity->MakeEntity(db);
    if(unaryPredicatesOpt)
    {
        unaryPredicatesOpt->Assert(db, e);
    }

    if(attributes)
    {
        attributes->Assert(db, e);
    }
}

void AST::AttributeList::Assert(Database &db, const ::Entity &e) const
{
    if(entityOpt)
    {
        ::Entity e2 = entityOpt->MakeEntity(db);
        BinaryTable & table = db.GetBinaryRelation(predicate->name);

        table.Add(e, e2);
    }

    if(list) list->Assert(db, e);
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
    std::cout << "TODO: Assert Datalog predicate " << predicate->name << ".\n";
}

void AST::Term::AssertRule(Database &db, Term &rhs) const
{
    // TODO
}

AST::NotImplementedEntity::NotImplementedEntity(AST::Node *n1, AST::Node *n2)
{
    std::unique_ptr<Node> node1(n1), node2(n2);
}

bool AST::NotImplementedEntity::IsVariable() const
{
    return false;
}
    
::Entity AST::NotImplementedEntity::MakeEntity(Database &db) const
{
    std::cerr << "Not implemented arithmetic\n";
    return db.CreateInt(-1);
}

