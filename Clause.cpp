#include "Clause.hpp"

AST::Variable::Variable(const char * name) : name(name)
{
}

AST::String::String(const std::string &p) : value(p) 
{ 
}

AST::TermIs::TermIs(Entity* entity, UnaryPredicateOrList* list)
    : entity(entity), list(list)
{
    if(entity && list)
    std::cout << "Got a full termis!\n";
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