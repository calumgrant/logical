#include "Clause.hpp"

AST::Node::~Node()
{
}

AST::Variable::Variable(const char * name) : name(name)
{
}

AST::String::String(const std::string &p) : value(p) 
{ 
}

AST::TermIs::TermIs(Entity* entity, UnaryPredicateOrList* list)
    : entity(entity), list(list)
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
    std::cout << "TODO: Assert the facts.\n";
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