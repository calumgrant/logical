#include <string>
#include <iostream>
#include <vector>

namespace AST
{
    class Node
    {

    };

    class Clause : public Node
    {
    };

    class UnaryRelation : public Clause
    {
    };

    class BinaryRelation : public Clause
    {

    };

    class Entity : public Clause
    {
    };

    class Variable : public Entity
    {
    public:
        Variable(const char * name);
        const std::string name;
    };

    class UnderscoreVariable : public Entity
    {
    };

    class AtEntity : public Entity
    {
    public:
        AtEntity(const char*v);
        const std::string value;
    };

    class String : public Entity
    {
    public:
        String(const std::string &p);
        const std::string value;
    };

    class Integer : public Entity
    {
    public:
        Integer(int i);
        const int value;
    };

    class Float : public Entity
    {
    public:
        Float(double v);
        const double value;
    };

    class Bool : public Entity{
    public:
        Bool(bool b);
        const bool value;
    };

    class Has : public Clause
    {
    };

    class Predicate : public Clause
    {
    public:
        Predicate(const char * name);
        std::string name;
    };

    class UnaryPredicateOrList : public Clause
    {
    };

    class UnaryPredicate : public UnaryPredicateOrList
    {
    public:
        UnaryPredicate(const char * name);
        std::string name;
    };

    class BinaryPredicate : public Predicate
    {
    public:
        BinaryPredicate(const char * name);
    };


    class UnaryPredicateList : public UnaryPredicateOrList
    {
    public:
        UnaryPredicateList(UnaryPredicate * pred);
        void Append(UnaryPredicate * pred);
        std::vector<std::unique_ptr<UnaryPredicate>> list;
    };

    class TermIs : public Clause
    {
    public:
        TermIs(Entity* entity, UnaryPredicateOrList* list);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
    };

    class Rule : public Clause
    {
    public:
        Rule(Clause * lhs, Clause * rhs);
        std::unique_ptr<Clause> lhs, rhs;
    };
}
