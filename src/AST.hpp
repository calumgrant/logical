#include <string>
#include <vector>

#include "Entity.hpp"
#include "SourceLocation.hpp"

enum class ComparatorType { lt, lteq, gt, gteq, eq, neq };

enum class IsType { is, isnot };

enum class HasType { has, hasnot, reaches, reachesno, comma };

std::ostream & operator<<(std::ostream &os, ComparatorType t);

namespace AST
{
    class Variable;
    class Value;
    class NamedVariable;
    class PragmaList;
    class EntityClause;

    class Visitor
    {
    public:
        virtual void OnPredicate(int nameId);
        virtual void OnUnaryPredicate(int nameId);
        virtual void OnBinaryPredicate(int nameId);

        virtual void OnVariable(int nameId);
        virtual void OnUnnamedVariable();

        virtual void OnValue(const ::Entity & value);
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
        Clause(const SourceLocation & loc);

        Clause * next;

        virtual void SetNext(Clause&);

        virtual void AssertFacts(Database &db) const =0;
        virtual std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation)=0;
        virtual std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation)=0;
        virtual void AddRule(Database &db, const std::shared_ptr<Evaluation>&)=0;

        // ?? const
        void Find(Database &db);
        
        void SetPragma(PragmaList * pragmas);
        std::unique_ptr<PragmaList> pragmas;
        
        const SourceLocation location;
    };

    class NotImplementedClause : public Clause
    {
    public:
        NotImplementedClause(const SourceLocation & loc, Node * =nullptr, Node* =nullptr, Node* =nullptr, Node* =nullptr);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
     };

    class Entity : public Node
    {
    public:
        Entity(const SourceLocation & loc);

        virtual const Variable * IsVariable() const =0;
        virtual const Value * IsValue() const =0;
        
        virtual void UnboundError(Database & db) const =0;

        /*
         Binds the variables in the entity in the compilation object.
         Returns the variable number of the variable representing this entity.
         Returns bound=true if the variable is already bound to a value.
         Returns bound=false if this is the first use of the variable in this evaluation branch.
         */
        virtual int BindVariables(Database & db, Compilation & c, bool & bound) =0;
        
        /*
            Produces an evaluation for this entity.
            If no evaluation is required, just returns `next`
            If evaluation is required, then the `next` node is used as the next node in the evaluation chain.
         */
        virtual std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const;
        
        const SourceLocation location;
    };

    class Variable : public Entity
    {
    public:
        Variable(const SourceLocation & location);
        const Variable * IsVariable() const override;
        virtual const NamedVariable * IsNamedVariable() const =0;
        const Value * IsValue() const override;
    };

    class NamedVariable : public Variable
    {
    public:
        NamedVariable(const SourceLocation & loc, int nameId);
        void Visit(Visitor&) const override;
        const NamedVariable * IsNamedVariable() const override;
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void UnboundError(Database & db) const override;

        const int nameId;
    };

    class UnnamedVariable : public Variable
    {
    public:
        UnnamedVariable(const SourceLocation & loc);
        void Visit(Visitor&) const override;
        const NamedVariable * IsNamedVariable() const override;
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void UnboundError(Database & db) const override;
    };

    class Value : public Entity
    {
    public:
        Value(const SourceLocation & loc, const ::Entity & entity);
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        const ::Entity &GetValue() const;
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void UnboundError(Database & db) const override;
        void Visit(Visitor &v) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const override;
    private:
        int slot;
        ::Entity value;
    };

    class ArithmeticEntity : public Entity
    {
    public:
        ArithmeticEntity(const SourceLocation & loc);
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        void UnboundError(Database & db) const override;
    };

    class NotImplementedEntity : public ArithmeticEntity
    {
    public:
        NotImplementedEntity(const SourceLocation & loc, Node *e1=nullptr, Node *e2=nullptr);
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        void Visit(Visitor&) const override;
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
    };

    class And : public Clause
    {
    public:
        And(const SourceLocation & loc, Clause *lhs, Clause *rhs);
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
        Or(const SourceLocation & loc, Clause *lhs, Clause *rhs);
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
        Not(const SourceLocation & loc, Clause * c);
        Not(const std::shared_ptr<Clause> & clause);  // Is this used?
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void SetNext(Clause&) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
        
    private:
        // Shared with All
        std::shared_ptr<Clause> clause;
    };

    class Comparator : public Clause
    {
    public:
        Comparator(const SourceLocation & loc, Entity * lhs, ComparatorType cmp, Entity * rhs);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
    private:
        std::unique_ptr<Entity> lhs, rhs;
        ComparatorType type;
    };

    class Range : public Clause
    {
    public:
        Range(const SourceLocation & loc, Entity * lowerBound, ComparatorType cmp1, Entity * entity, ComparatorType cmp2, Entity * upperBound);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
    private:
        std::unique_ptr<Entity> lowerBound, entity, upperBound;
        ComparatorType cmp1, cmp2;
    };

    class Predicate : public Node
    {
    public:
        Predicate(int nameId);
        void Visit(Visitor&) const override;
        const int nameId;
    };

    class UnaryPredicate : public Node
    {
    public:
        UnaryPredicate(int nameId);
        void Assert(Database &db, const ::Entity &e) const;
        void Visit(Visitor&) const override;
        const int nameId;
    };

    class BinaryPredicate : public Predicate
    {
    public:
        BinaryPredicate(int nameId);
        void Visit(Visitor&) const override;
    };

    class UnaryPredicateList : public Node
    {
    public:
        UnaryPredicateList(UnaryPredicate * pred);
        void Append(UnaryPredicate * pred);
        void Assert(Database &db, const ::Entity &e) const;
        void Visit(Visitor&) const override;
        std::vector< std::unique_ptr<UnaryPredicate> > list;
    };

    struct Attribute
    {
        Attribute(BinaryPredicate *predicate, Entity *entityOpt);
        Attribute(Entity *entityOpt);
        Attribute(EntityClause * ec);
        
        std::vector<std::unique_ptr<BinaryPredicate>> predicates;
        std::unique_ptr<Entity> entityOpt;
        std::unique_ptr<EntityClause> entityClauseOpt;
        std::unique_ptr<Attribute> withOpt;
        HasType withType;
        
        void SetWith(Attribute * with, HasType withType);
        
        int slot;
        bool bound;
        
        void AddFirst(BinaryPredicate * pred);
    };

    class AttributeList : public Node
    {
    public:
        AttributeList(Attribute * attribute);
        AttributeList(BinaryPredicate * predicate, Entity * entityOpt);
        
        void Add(BinaryPredicate * predicate, Entity * entityOpt);
        void Add(Attribute*);
        
        std::vector<Attribute> attributes;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database & db, Compilation &c, int slot, bool alreadyBound, Clause *next, HasType has, const PredicateName & name, Relation & relation);

        CompoundName GetCompoundName() const;
    };

    /*
     Any clause of the form:
        large mouse X
        large mouse X is small dog
        X has name "Snoopy", age 10
     */
    class EntityClause : public Clause
    {
    public:
        EntityClause(const SourceLocation & loc, Entity* entity, UnaryPredicateList* predicates, AttributeList * attributes = nullptr, IsType is = IsType::is, HasType has=HasType::has);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;
        void AddFirst(UnaryPredicate*);
        void SetAttributes(AttributeList*, HasType has);
    private:
        std::shared_ptr<Evaluation> WritePredicates(Database &db, Compilation &c, int slot);
        
        const IsType is;
        HasType has;
        std::unique_ptr<Entity> entity;
        std::unique_ptr<UnaryPredicateList> predicates;
        std::unique_ptr<AttributeList> attributes;
        
        std::shared_ptr<Table> newEntityTable;
        
        void AssertEntity(Database &db, ::Entity e) const;
        
        PredicateName GetPredicateName() const;
        
        // Holds if this predicate should be "decomposed"
        // (i.e. `X has foo Y, bar Z` becomes `X has foo Y and X has bar Z`)
        bool Decompose(const Database & db) const;
        EvaluationPtr CompileDecomposed(Database & db, Compilation & compilation, int slot, bool lhsBound);
    };

    class EntityIs : public EntityClause
    {
    public:
        EntityIs(const SourceLocation & loc, Entity* entity, UnaryPredicateList* list, IsType is);
    };

    class EntityIsPredicate : public EntityClause
    {
    public:
        EntityIsPredicate(const SourceLocation & loc, Entity* entity, UnaryPredicateList* list);
    };

    class EntityHasAttributes : public EntityClause
    {
    public:
        EntityHasAttributes(const SourceLocation & loc, UnaryPredicateList * unarypredicatesOpt, Entity*entity, AttributeList*attributes, HasType has);
    };

    class NewEntity : public EntityClause
    {
    public:
        NewEntity(const SourceLocation & loc, UnaryPredicate * pred, AttributeList * attributes);
    };

    class EntityList : public Node
    {
    public:
        EntityList(Entity*);
        void Add(Entity*);
        void AddFirst(Entity*);
        void Visit(Visitor&) const override;
        std::vector< std::unique_ptr<Entity> > entities;
    };

    class DatalogPredicate : public Clause
    {
    public:
        DatalogPredicate(const SourceLocation & loc, Predicate * predicate, EntityList * entityListOpt);
        void AssertFacts(Database &db) const override;
        void Visit(Visitor&) const override;
        void AddRule(Database &db, const std::shared_ptr<Evaluation>&) override;

        std::shared_ptr<Evaluation> Compile(Database &db, Compilation & compilation) override;
        std::shared_ptr<Evaluation> CompileLhs(Database &db, Compilation &compilation) override;
    private:
        PredicateName GetPredicateName(Database & db) const;
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
        
        void SetPragma(PragmaList * pragmas);
        std::unique_ptr<PragmaList> pragmas;
    };

    class BinaryArithmeticEntity : public ArithmeticEntity
    {
    protected:
        BinaryArithmeticEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void Visit(Visitor &v) const override;

        std::unique_ptr<Entity> lhs, rhs;
        int resultSlot, lhsSlot, rhsSlot;
    };

    class AddEntity : public BinaryArithmeticEntity
    {
    public:
        AddEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const override;
    };

    class SubEntity : public BinaryArithmeticEntity
    {
    public:
        SubEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const override;
    };

    class MulEntity : public BinaryArithmeticEntity
    {
    public:
        MulEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const override;
    };

    class DivEntity : public BinaryArithmeticEntity
    {
    public:
        DivEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &, const std::shared_ptr<Evaluation> & next) const override;
    };

    class ModEntity : public BinaryArithmeticEntity
    {
    public:
        ModEntity(const SourceLocation & loc, Entity * lhs, Entity * rhs);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const override;
    };

    class NegateEntity : public ArithmeticEntity
    {
    public:
        NegateEntity(const SourceLocation & loc, Entity * e);
        
        const Variable * IsVariable() const override;
        const Value * IsValue() const override;
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void Visit(Visitor &v) const override;
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation&, const std::shared_ptr<Evaluation> & next) const override;

    private:
        std::unique_ptr<Entity> entity;
        int resultSlot, slot1;
    };

    class Aggregate : public ArithmeticEntity
    {
    protected:
        Aggregate(const SourceLocation & loc, Entity *e, Entity * value, Clause *c);
        std::unique_ptr<Entity> entity, value;
        std::unique_ptr<Clause> clause;
        
        int BindVariables(Database & db, Compilation &c, bool & bound) override;
        void Visit(Visitor &v) const override;
        
        int slot;
    };

    class Count : public Aggregate
    {
    public:
        Count(const SourceLocation & loc, Entity *e, Clause *c);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &c, const std::shared_ptr<Evaluation> & next) const override;
    };

    class Sum : public Aggregate
    {
    public:
        Sum(const SourceLocation & loc, Entity * entity, Entity * value, Clause * clause);
        std::shared_ptr<Evaluation> Compile(Database &db, Compilation &c, const std::shared_ptr<Evaluation> & next) const override;
    };

    Clause * MakeAll(Clause * ifPart, Clause * thenPart);

    class PragmaList : public Node
    {
    public:
        PragmaList(StringId p);
        void Add(StringId p);
        void Visit(Visitor&) const override;
    private:
        std::vector<StringId> parts;
    };
}
