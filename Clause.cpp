#include "Clause.hpp"
#include "Database.hpp"

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

AST::AtEntity::AtEntity(const char * v) : value(v)
{   
}

AST::Rule::Rule(Clause * lhs, Clause * rhs) : lhs(lhs), rhs(rhs)
{
}

void AST::TermIs::AssertFacts(Database &db)
{
    list->Assert(db, entity->MakeEntity(db));
}

void AST::TermIsPredicate::AssertFacts(Database &db)
{
    ::Entity e(entity->MakeEntity(db));
    list->Assert(db, e);
    predicate->Assert(db, e);
}


AST::NotImplementedClause::NotImplementedClause(Node *a, Node *b, Node *c, Node *d)
{
    delete a;
    delete b;
    delete c;
    delete d;
}

void AST::NotImplementedClause::AssertFacts(Database & db)
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

bool AST::Bool::IsVariable() const { return false; }

Entity AST::Bool::MakeEntity(Database &db) const
{
    return db.Create(value);
}

bool AST::Float::IsVariable() const { return false; }

Entity AST::Float::MakeEntity(Database &db) const
{
    return db.Create(value);
}

bool AST::String::IsVariable() const { return false; }

Entity AST::String::MakeEntity(Database &db) const
{
    return db.CreateString(value);
}

bool AST::Integer::IsVariable() const { return false; }

Entity AST::Integer::MakeEntity(Database &db) const
{
    return db.CreateInt(value);
}

bool AST::AtEntity::IsVariable() const { return false; }

Entity AST::AtEntity::MakeEntity(Database &db) const
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

AST::And::And(Clause *lhs, Clause *rhs) : lhs(lhs), rhs(rhs)
{
}

void AST::And::AssertFacts(Database &db)
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

void AST::EntityHasAttributes::AssertFacts(Database &db)
{
    std::cout << "TODO: Assert attribute facts\n";
}
