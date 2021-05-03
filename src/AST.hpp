#include <string>
#include <vector>

class Database;
class Entity;
class Compilation;
class Evaluation;

enum class ComparatorType { lt, lteq, gt, gteq, eq, neq };

namespace AST
{
    class Variable;
    class Value;
    class NamedVariable;

    class Visitor
    {
    public:
        virtual void OnPredicate(const std::string&);
        virtual void OnUnaryPredicate(const std::string&);
        virtual void OnBinaryPredicate(const std::string&);

        virtual void OnVariable(const std::string&);
        virtual void OnUnnamedVariable();

        virtual void OnString(const std::string & value);
        virtual void OnAtString(const std::string & value);
        virtual void OnInteger(int value);
        virtual void OnFloat(double value);
        virtual void OnBool(bool value);
    };

    class Node
    {
    public:
        virtual ~Node();
        virtual void Visit(Visitor&) const =0;
    };

    class Clause : public Node
    {
    public:
        Clause();
        virtual void AssertFacts(Database &db) const =0;

        // const?
        Clause * next;
        virtual void SetNext(Clause&);

        // ?? Const
        virtual std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation)=0;
        virtual std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation)=0;
        virtual void AddRule(Database &db, const std::shared_ptr<Evaluation>&)=0;

        // ?? const
        void Find(Database &db);
    };

    class NotImplementedClause : public Clause
    {
    public:
        NotImplementedClause(Node * =nullptr, Node* =nullptr, Node* =nullptr, Node* =nullptr);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
     };

    class Entity : public Node
    {
    public:
        virtual const Variable * IsVariable() const =0;
        virtual const Value * IsValue() const =0;
        virtual int CompileEntity(Database & db, Compilation &c, bool & bound) const =0;
    };

    class Variable : public Entity
    {
    public:
        const Variable * IsVariable() const override;
        virtual const NamedVariable * IsNamedVariable() const =0;
        const Value * IsValue() const override;
    };

    class NamedVariable : public Variable
    {
    public:
        NamedVariable(const char * name);
        void Visit(Visitor&) const override;
        const NamedVariable * IsNamedVariable() const override;
        int CompileEntity(Database & db, Compilation &c, bool & bound) const override;

        const std::string name;
    };

    class UnnamedVariable : public Variable
    {
        void Visit(Visitor&) const override;
        const NamedVariable * IsNamedVariable() const override;
        int CompileEntity(Database & db, Compilation &c, bool & bound) const override;
    };

    class Value : public Entity
    {
    public:
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        virtual ::Entity MakeEntity(Database &db) const =0;
        int CompileEntity(Database & db, Compilation &c, bool & bound) const override;
    };

    class AtString : public Value
    {
    public:
        AtString(const char*v);
        ::Entity MakeEntity(Database &db) const override;
        void Visit(Visitor&) const override;
        const std::string value;
    };

    class String : public Value
    {
    public:
        String(const std::string &p);
        ::Entity MakeEntity(Database &db) const override;
        void Visit(Visitor&) const override;
        const std::string value;
    };

    class Integer : public Value
    {
    public:
        Integer(int i);
        ::Entity MakeEntity(Database &db) const override;
        const int value;
        void Visit(Visitor&) const override;
    };

    class Float : public Value
    {
    public:
        Float(double v);
        ::Entity MakeEntity(Database &db) const override;
        const double value;
        void Visit(Visitor&) const override;
    };

    class Bool : public Value
    {
    public:
        Bool(bool b);
        ::Entity MakeEntity(Database &db) const override;
        const bool value;
        void Visit(Visitor&) const override;
    };

    class ArithmeticEntity : public Entity
    {
    };

    class NotImplementedEntity : public ArithmeticEntity
    {
    public:
        NotImplementedEntity(Node *e1=nullptr, Node *e2=nullptr);
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        void Visit(Visitor&) const override;
        int CompileEntity(Database & db, Compilation &c, bool & bound) const override;
    };

    class And : public Clause
    {
    public:
        And(Clause *lhs, Clause *rhs);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void SetNext(Clause&) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
        std::unique_ptr<Clause> lhs, rhs;
    };

    class Or : public Clause
    {
    public:
        Or(Clause *lhs, Clause *rhs);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void SetNext(Clause&) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
        std::unique_ptr<Clause> lhs, rhs;
    };

    class Not : public Clause
    {
    public:
        Not(Clause * c);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void SetNext(Clause&) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
        
    private:
        std::unique_ptr<Clause> clause;
    };

    class Comparator : public Clause
    {
    public:
        Comparator(Entity * lhs, ComparatorType cmp, Entity * rhs);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
    private:
        std::unique_ptr<Entity> lhs, rhs;
        ComparatorType type;
    };

    class Predicate : public Node
    {
    public:
        Predicate(const char * name);
        void Visit(Visitor&) const override;
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
        void Visit(Visitor&) const override;
        std::string name;
    };

    class BinaryPredicate : public Predicate
    {
    public:
        BinaryPredicate(const char * name);
        void Visit(Visitor&) const override;
    };

    class UnaryPredicateList : public UnaryPredicateOrList
    {
    public:
        UnaryPredicateList(UnaryPredicate * pred);
        void Append(UnaryPredicate * pred);
        void Assert(Database &db, const ::Entity &e) const override;
        void Visit(Visitor&) const override;
        std::vector< std::unique_ptr<UnaryPredicate> > list;
    };

    class EntityIs : public Clause
    {
    public:
        EntityIs(Entity* entity, UnaryPredicateOrList* list);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
    private:
        std::shared_ptr<Evaluation> WritePredicates(Database &db, int slot);
        
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
    };

    class EntityIsPredicate : public Clause
    {
    public:
        EntityIsPredicate(Entity* entity, UnaryPredicateOrList* list, UnaryPredicate * p);
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateOrList> list;
        std::unique_ptr<UnaryPredicate> predicate;
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
    };

    class AttributeList : public Node
    {
    public:
        AttributeList(BinaryPredicate * predicate, Entity * entityOpt, AttributeList* listOpt);
        std::unique_ptr<AttributeList> listOpt;  // ?? Not sure if this should be nullable
        std::unique_ptr<BinaryPredicate> predicate;
        std::unique_ptr<Entity> entityOpt;

        void Assert(Database &db, const ::Entity &e) const;
        void Visit(Visitor&) const override;
    };

    class EntityHasAttributes : public Clause
    {
    public:
        EntityHasAttributes(UnaryPredicateOrList * unarypredicatesOpt, Entity*entity, AttributeList*attributes);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;

        std::unique_ptr<UnaryPredicateOrList> unaryPredicatesOpt;
        std::unique_ptr<Entity> entity;
        std::unique_ptr<AttributeList> attributes;
    };

    class EntityList : public Node
    {
    public:
        EntityList(Entity*);
        void Add(Entity*);
        void Visit(Visitor&) const override;
        std::vector< std::unique_ptr<Entity> > entities;
    };

    class DatalogPredicate : public Clause
    {
    public:
        DatalogPredicate(Predicate * predicate, EntityList * entityListOpt);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;

        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        std::unique_ptr<Predicate> predicate;
        std::unique_ptr<EntityList> entitiesOpt;
    };

    class Rule : public Node
    {
    public:
        Rule(Clause * lhs, Clause * rhs);
        void Compile(Database &db);
        void Visit(Visitor&) const override;
        std::unique_ptr<Clause> lhs, rhs;
    };
}
