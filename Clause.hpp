#include <string>
#include <iostream>
#include <vector>

class Database;
class Entity;

namespace AST
{
    class Node
    {
    public:
        virtual ~Node();
    };

    class Clause : public Node
    {
    public:
        virtual void AssertFacts(Database &db)=0;
    };

    class NotImplementedClause : public Clause
    {
    public:
        NotImplementedClause(Node * =nullptr, Node* =nullptr, Node* =nullptr, Node* =nullptr);
        void AssertFacts(Database &db) override;
    };

    class Entity : public Node
    {
    public:
        virtual bool IsVariable() const =0;
        virtual ::Entity MakeEntity(Database &db) const =0;
    };

    class Variable : public Entity
    {
    public:
        bool IsVariable() const override;
    };

    class NamedVariable : public Variable
    {
    public:
        NamedVariable(const char * name);
        ::Entity MakeEntity(Database &db) const override;
        const std::string name;
    };

    class UnnamedVariable : public Variable
    {
        ::Entity MakeEntity(Database &db) const override;
    };

    class AtEntity : public Entity
    {
    public:
        AtEntity(const char*v);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
        const std::string value;
    };

    class String : public Entity
    {
    public:
        String(const std::string &p);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
        const std::string value;
    };

    class Integer : public Entity
    {
    public:
        Integer(int i);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
        const int value;
    };

    class Float : public Entity
    {
    public:
        Float(double v);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
        const double value;
    };

    class Bool : public Entity{
    public:
        Bool(bool b);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
        const bool value;
    };

    class Has : public Clause
    {
    };

    class And : public Clause
    {
    public:
        And(Clause *lhs, Clause *rhs);
        void AssertFacts(Database &db) override;
        std::unique_ptr<Clause> lhs, rhs;
    };

    class Predicate : public Node
    {
    public:
        Predicate(const char * name);
        std::string name;
    };

    class UnaryPredicateOrList : public Node
    {
    public:
        virtual void Assert(Database &db, const ::Entity &e) const =0;
    };

    class UnaryPredicate : public UnaryPredicateOrList
    {
    public:
        UnaryPredicate(const char * name);
        void Assert(Database &db, const ::Entity &e) const override;
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
        void Assert(Database &db, const ::Entity &e) const override;
        std::vector<std::unique_ptr<UnaryPredicate>> list;
    };

    class TermIs : public Clause
    {
    public:
        TermIs(Entity* entity, UnaryPredicateOrList* list);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
        void AssertFacts(Database &db) override;
    };

    class TermIsPredicate : public Clause
    {
    public:
        TermIsPredicate(Entity* entity, UnaryPredicateOrList* list, UnaryPredicate * p);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
        std::unique_ptr<UnaryPredicate> predicate;
        void AssertFacts(Database &db) override;
    };

    class AttributeList : public Node
    {
    public:
        AttributeList(BinaryPredicate * predicate, Entity * entityOpt, AttributeList* list);
        std::unique_ptr<AttributeList> list;
        std::unique_ptr<BinaryPredicate> predicate;
        std::unique_ptr<Entity> entityOpt;
    };

    class EntityHasAttributes : public Clause
    {
    public:
        EntityHasAttributes(UnaryPredicateOrList * unarypredicatesOpt, Entity*entity, AttributeList*attributes);
        void AssertFacts(Database &db) override;
        std::unique_ptr<UnaryPredicateOrList> unaryPredicatesOpt;
        std::unique_ptr<Entity> entity;
        std::unique_ptr<AttributeList> attributes;
    };

    class Rule : public Clause
    {
    public:
        Rule(Clause * lhs, Clause * rhs);
        std::unique_ptr<Clause> lhs, rhs;
    };
}
