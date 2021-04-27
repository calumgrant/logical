#include <string>
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

    class Term : public Node
    {
    public:
        virtual void AssertFacts(Database &db) const =0;
        void AssertRule(Database &db, Term&rhs) const;
    };

    class NotImplementedTerm : public Term
    {
    public:
        NotImplementedTerm(Node * =nullptr, Node* =nullptr, Node* =nullptr, Node* =nullptr);
        void AssertFacts(Database &db) const override;
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

    class Value : public Entity
    {
    public:
        bool IsVariable() const override;
    };

    class AtString : public Value
    {
    public:
        AtString(const char*v);
        ::Entity MakeEntity(Database &db) const override;
        const std::string value;
    };

    class String : public Value
    {
    public:
        String(const std::string &p);
        ::Entity MakeEntity(Database &db) const override;
        const std::string value;
    };

    class Integer : public Value
    {
    public:
        Integer(int i);
        ::Entity MakeEntity(Database &db) const override;
        const int value;
    };

    class Float : public Value
    {
    public:
        Float(double v);
        ::Entity MakeEntity(Database &db) const override;
        const double value;
    };

    class Bool : public Value
    {
    public:
        Bool(bool b);
        ::Entity MakeEntity(Database &db) const override;
        const bool value;
    };

    class ArithmeticEntity : public Entity
    {
    };

    class NotImplementedEntity : public ArithmeticEntity
    {
    public:
        NotImplementedEntity(Node *e1=nullptr, Node *e2=nullptr);
        bool IsVariable() const override;
        ::Entity MakeEntity(Database &db) const override;
    };

    class And : public Term
    {
    public:
        And(Term *lhs, Term *rhs);
        void AssertFacts(Database &db) const override;
        std::unique_ptr<Term> lhs, rhs;
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
        std::vector< std::unique_ptr<UnaryPredicate> > list;
    };

    class TermIs : public Term
    {
    public:
        TermIs(Entity* entity, UnaryPredicateOrList* list);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
        void AssertFacts(Database &db) const override;
    };

    class TermIsPredicate : public Term
    {
    public:
        TermIsPredicate(Entity* entity, UnaryPredicateOrList* list, UnaryPredicate * p);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
        std::unique_ptr<UnaryPredicate> predicate;
        void AssertFacts(Database &db) const override;
    };

    class AttributeList : public Node
    {
    public:
        AttributeList(BinaryPredicate * predicate, Entity * entityOpt, AttributeList* list);
        std::unique_ptr<AttributeList> list;
        std::unique_ptr<BinaryPredicate> predicate;
        std::unique_ptr<Entity> entityOpt;

        void Assert(Database &db, const ::Entity &e) const;
    };

    class EntityHasAttributes : public Term
    {
    public:
        EntityHasAttributes(UnaryPredicateOrList * unarypredicatesOpt, Entity*entity, AttributeList*attributes);
        void AssertFacts(Database &db) const override;
        std::unique_ptr<UnaryPredicateOrList> unaryPredicatesOpt;
        std::unique_ptr<Entity> entity;
        std::unique_ptr<AttributeList> attributes;
    };

    class EntityList : public Node
    {
    public:
        EntityList(Entity*);
        void Add(Entity*);
        std::vector< std::unique_ptr<Entity> > entities;
    };

    class DatalogPredicate : public Term
    {
    public:
        DatalogPredicate(Predicate * predicate, EntityList * entityListOpt);
        void AssertFacts(Database &db) const override;
        std::unique_ptr<Predicate> predicate;
        std::unique_ptr<EntityList> entitiesOpt;
    };
}
