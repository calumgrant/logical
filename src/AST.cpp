#include "AST.hpp"
#include "Database.hpp"
#include <iostream>

AST::Node::~Node()
{
}

AST::NamedVariable::NamedVariable(const SourceLocation & loc, int nameId) : Variable(loc), nameId(nameId)
{
}

AST::Value::Value(const SourceLocation & loc, const ::Entity &v) : Entity(loc), value(v)
{
}

AST::UnnamedVariable::UnnamedVariable(const SourceLocation &loc) : Variable(loc)
{
}

AST::Variable::Variable(const SourceLocation & loc) : Entity(loc)
{
}

AST::EntityClause::EntityClause(const SourceLocation & loc, Entity* entity, UnaryPredicateList* list, AttributeList * attributes, IsType is, HasType has)
    : Clause(loc), entity(entity), is(is), has(has), predicates(list), /* isPredicates(isList), */ attributes(attributes)
{
}

AST::EntityIs::EntityIs(const SourceLocation & loc, Entity * entity, UnaryPredicateList * list, IsType is) : EntityClause(loc, entity, list, nullptr, is)
{
}

AST::EntityIsPredicate::EntityIsPredicate(const SourceLocation & loc, Entity* entity, UnaryPredicateList* list) :
    EntityClause(loc, entity, list)
{
}


AST::Predicate::Predicate(int nameId) : nameId(nameId) { }

AST::UnaryPredicate::UnaryPredicate(int nameId) : nameId(nameId) { }

AST::BinaryPredicate::BinaryPredicate(int nameId) : Predicate(nameId) { }

AST::UnaryPredicateList::UnaryPredicateList(UnaryPredicate * p)
{
    Append(p);
}

void AST::UnaryPredicateList::Append(UnaryPredicate * p)
{
    list.push_back(std::unique_ptr<UnaryPredicate>(p));
}

void AST::EntityClause::AssertFacts(Database &db) const
{
    if(!entity)
    {
        auto e = db.NewEntity();
        AssertEntity(db, e);
        return;
    }
    
    auto v = entity->IsValue();
    if(v)
    {
        auto e = v->GetValue();
        AssertEntity(db, e);
    }
    else if(auto var = entity->IsVariable())
    {
        auto nv = var->IsNamedVariable();
        if(nv)
        {
            // Treat variables as strings when asserting facts.
            auto string = ::Entity(EntityType::String, nv->nameId);
            AssertEntity(db, string);
        }
        else
        {
            // Create an empty relation in the database.
            // This is used to prevent "undefined relation" errors by indicating that this
            // is a legitimate relation.
            PredicateName name = GetPredicateName();
            auto & relation = db.GetRelation(name);
            relation.allowEmpty = true;

        }
    }
    else
        entity->UnboundError(db);
}

void AST::EntityClause::AssertEntity(Database &db, ::Entity e) const
{
    PredicateName name = GetPredicateName();
    auto & relation = db.GetRelation(name);
    
    std::vector<::Entity> entities(name.arity);
    entities[0] = e;
    
    if(attributes)
    {
        int index = 0;
        for(auto & attribute : attributes->attributes)
        {
            if(attribute.entityOpt)
            {
                ::Entity e;
                if(auto v = attribute.entityOpt->IsValue())
                {
                    e = v->GetValue();
                }
                else if(auto v2 = attribute.entityOpt->IsVariable())
                {
                    if(auto v3 = v2->IsNamedVariable())
                    {
                         e = ::Entity(EntityType::String, v3->nameId);
                    }
                    else
                    {
                        attribute.entityOpt->UnboundError(db);
                        return;
                    }
                }
                else
                {
                    attribute.entityOpt->UnboundError(db);
                    return;
                }

                entities[name.attributes.mapFromInputToOutput[index]+1] = e;
            }
            // TODO: Else report error
                         
            ++index;
        }
        
        if(attributes->attributes.size() > name.attributes.parts.size())
        {
            // TODO: A better error
            std::cout << "Duplicate attribute is invalid\n";
            db.ReportUserError();
        }
    }

    relation.OnStartRunningRules();
    relation.Add(location, &entities[0]);
    relation.OnStopRunningRules();
}


AST::NotImplementedClause::NotImplementedClause(const SourceLocation & loc, Node *a, Node *b, Node *c, Node *d) : Clause(loc)
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
    PredicateName name(1, nameId);
    db.GetRelation(name).Add(&e);
}

void AST::UnaryPredicateList::Assert(Database &db, const ::Entity &e) const
{
    for(auto &l : list)
        l->Assert(db, e);
}

const AST::Variable * AST::Value::IsVariable() const { return nullptr; }

const AST::Value * AST::Value::IsValue() const { return this; }

const AST::Variable * AST::Variable::IsVariable() const { return this; }

AST::And::And(const SourceLocation & loc, Clause *lhs, Clause *rhs) : Clause(loc), lhs(lhs), rhs(rhs)
{
}

void AST::And::AssertFacts(Database &db) const
{
    lhs->AssertFacts(db);
    rhs->AssertFacts(db);
}

AST::AttributeList::AttributeList(BinaryPredicate * predicate, Entity * entityOpt)
{
    Add(predicate, entityOpt);
}

AST::EntityHasAttributes::EntityHasAttributes(const SourceLocation & loc, UnaryPredicateList * unaryPredicatesOpt, Entity * entity, AttributeList * attributes, HasType has) :
    EntityClause(loc, entity, unaryPredicatesOpt, attributes, IsType::is, has)
{
}

AST::EntityList::EntityList(Entity *e)
{
    Add(e);
}

void AST::EntityList::Add(Entity *e)
{
    entities.push_back(std::unique_ptr<Entity>(e));
}

AST::DatalogPredicate::DatalogPredicate(const SourceLocation & loc, Predicate * predicate, EntityList * entityListOpt) :
    Clause(loc), predicate(predicate), entitiesOpt(entityListOpt)
{
}

PredicateName AST::DatalogPredicate::GetPredicateName(Database & db) const
{
    // Unpack a name from a Datalog name to a Logical name
    // For example: string:length(_,_) becomes string _ has length _.
    
    PredicateName name;
    
    name.arity = entitiesOpt ? entitiesOpt->entities.size() : 0;
    
    if(entitiesOpt)
    {
        auto str = db.GetString(predicate->nameId);
        
        int colons = 0;
        for(auto s = str; *s; ++s)
        {
            if(*s==':') ++colons;
        }
                
        if(colons>0 && name.arity>1 && colons+1 >= name.arity)
        {
            // We need to unpack this name
            int firstColon = colons + 2 - name.arity;
            int c=0;
            std::string latest;
            std::vector<int> attributes;
            
            for(auto i=str; *i; ++i)
            {
                if(*i==':')
                {
                    ++c;
                    if(c == firstColon)
                    {
                        name.objects.parts.push_back(db.GetStringId(latest.c_str()));
                        latest="";
                    }
                    else if(c>firstColon)
                    {
                        attributes.push_back(db.GetStringId(latest.c_str()));
                        latest="";
                    }
                    else
                    {
                        latest += *i;
                    }
                }
                else
                {
                    latest += *i;
                }
            }

            attributes.push_back(db.GetStringId(latest.c_str()));
            
            name.attributes = CompoundName(attributes);
        }
        else
        {
            name.objects.parts.push_back(predicate->nameId);
        }
    }
    else
    {
        name.objects.parts.push_back(predicate->nameId);
    }
    return name;
}

void AST::DatalogPredicate::AssertFacts(Database &db) const
{
    auto name = GetPredicateName(db);
    
    switch(name.arity)
    {
    case 0:
        db.GetRelation(name).Add(nullptr);
        return;
            /*
    case 1:
        {
            auto v = entitiesOpt->entities[0]->IsValue();
            if(v)
            {
                ::Entity e = v->GetValue();
                db.GetRelation(name).Add(&e);
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
                ::Entity row[2] = { v0->GetValue(), v1->GetValue() };
                db.GetRelation(name).Add(row);
            }

            return;
        }
             */
    }

    auto & relation = db.GetRelation(name);
    
    std::vector<::Entity> row(entitiesOpt->entities.size());

    for(int i=0; i<entitiesOpt->entities.size(); ++i)
    {
        auto & v = entitiesOpt->entities[i];
        auto w = v->IsValue();
        if(w)
        {
            auto j = name.MapArgument(i);
            row[j]= w->GetValue();
        }
        else
        {
            relation.allowEmpty = true;
            return; // Skip this row.
        }
    }
    
    relation.OnStartRunningRules();
    relation.Add(location, row.data());
    relation.OnStopRunningRules();
}

AST::NotImplementedEntity::NotImplementedEntity(const SourceLocation & loc, AST::Node *n1, AST::Node *n2) : ArithmeticEntity(loc)
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
    v.OnVariable(nameId);
}

void AST::UnaryPredicate::Visit(Visitor&v) const
{
    v.OnUnaryPredicate(nameId);
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

void AST::Visitor::OnVariable(int)
{
}

void AST::Visitor::OnUnaryPredicate(int)
{
}

void AST::Visitor::OnUnnamedVariable()
{
}

void AST::EntityClause::SetAttributes(AttributeList * a, HasType has)
{
    attributes = std::unique_ptr<AttributeList>(a);
    this->has = has;
}

void AST::EntityClause::Visit(Visitor&v) const
{
    if(entity) entity->Visit(v);
    if(predicates) predicates->Visit(v);
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

void AST::NotImplementedEntity::Visit(Visitor&v) const
{
}

void AST::EntityList::Visit(Visitor&v) const
{
    for(auto &e : entities)
        e->Visit(v);
}

void AST::Predicate::Visit(Visitor&v) const
{
    v.OnPredicate(nameId);
}

void AST::AttributeList::Visit(Visitor &v) const
{
    for(auto & a : attributes)
    {
        for(auto & p : a.predicates)
            p->Visit(v);
        if(a.entityOpt) a.entityOpt->Visit(v);
    }
}

void AST::Visitor::OnPredicate(int name)
{
}

void AST::BinaryPredicate::Visit(Visitor&v) const
{
    v.OnBinaryPredicate(nameId);
}

void AST::Visitor::OnBinaryPredicate(int name)
{
}

void AST::Rule::Visit(Visitor&v) const
{
    lhs->Visit(v);
    rhs->Visit(v);
}

AST::Clause::Clause(const SourceLocation & loc) : location(loc), next(nullptr)
{
}

void AST::Clause::SetNext(Clause & n)
{
    next = &n;
}

AST::Or::Or(const SourceLocation & loc, Clause * l, Clause *r) : Clause(loc), lhs(l), rhs(r)
{
}

AST::Not::Not(const SourceLocation & loc, Clause *c) : Clause(loc), clause(c)
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

AST::Comparator::Comparator(const SourceLocation & loc, Entity * lhs, ComparatorType type, Entity * rhs) : Clause(loc), lhs(lhs), type(type), rhs(rhs)
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

AST::Range::Range(const SourceLocation & loc, Entity * lb, ComparatorType cmp1, Entity *e, ComparatorType cmp2, Entity * ub) : Clause(loc),
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

AST::NegateEntity::NegateEntity(const SourceLocation & loc, Entity * e) : ArithmeticEntity(loc), entity(e)
{
}

AST::BinaryArithmeticEntity::BinaryArithmeticEntity(const SourceLocation & loc, Entity *l, Entity *r) : ArithmeticEntity(loc), lhs(l), rhs(r)
{
}

AST::AddEntity::AddEntity(const SourceLocation & loc, Entity *l, Entity *r) : BinaryArithmeticEntity(loc, l, r)
{
}

AST::SubEntity::SubEntity(const SourceLocation & loc, Entity *l, Entity *r) : BinaryArithmeticEntity(loc, l, r)
{
}

AST::MulEntity::MulEntity(const SourceLocation & loc, Entity *l, Entity *r) : BinaryArithmeticEntity(loc, l, r)
{
}

AST::DivEntity::DivEntity(const SourceLocation & loc, Entity *l, Entity *r) : BinaryArithmeticEntity(loc, l, r)
{
}

AST::ModEntity::ModEntity(const SourceLocation & loc, Entity *l, Entity *r) : BinaryArithmeticEntity(loc, l, r)
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

AST::Count::Count(const SourceLocation & loc, Entity *e, Clause *c) : Aggregate(loc, e, nullptr, c)
{
}

void AST::Aggregate::Visit(Visitor &v) const
{
    entity->Visit(v);
    clause->Visit(v);
}

AST::Aggregate::Aggregate(const SourceLocation & loc, Entity *e, Entity * v, Clause *c) :
    ArithmeticEntity(loc), entity(e), value(v), clause(c)
{
}

AST::Aggregate::Aggregate(const SourceLocation & loc, EntityList *e, Clause *c) :
    ArithmeticEntity(loc), clause(c)
{
    if(e->entities.size()>0)
        entity = std::move(e->entities[0]);
    if(e->entities.size()>1)
        value = std::move(e->entities[1]);
    delete e;
}


void AST::NamedVariable::UnboundError(Database &db) const
{
    db.UnboundError(db.GetString(nameId), location);
}

void AST::UnnamedVariable::UnboundError(Database &db) const
{
    db.UnboundError("_", location);
}

void AST::ArithmeticEntity::UnboundError(Database &db) const
{
    db.UnboundError("arithmetic_expression", location);
}

void AST::Value::UnboundError(Database &db) const
{
    db.UnboundError("??", location);  // Shouldn't really get here
}

AST::Sum::Sum(const SourceLocation & loc, Entity * entity, Entity * value, Clause * clause) : Aggregate(loc, entity, value, clause)
{
}

AST::Sum::Sum(const SourceLocation & loc, AST::EntityList * e, AST::Clause *c) : Aggregate(loc, e, c)
{
}

AST::Count::Count(const SourceLocation & loc, AST::EntityList * e, AST::Clause *c) : Aggregate(loc, e, c)
{
}

void AST::Visitor::OnValue(const ::Entity&)
{
}

void AST::Value::Visit(Visitor &v) const
{
    v.OnValue(value);
}

const ::Entity & AST::Value::GetValue() const
{
    return value;
}

void AST::AttributeList::Add(BinaryPredicate * p, Entity *e)
{
    attributes.push_back(Attribute(p,e));
}

AST::AttributeList::AttributeList(Attribute *a)
{
    Add(a);
}

AST::Attribute::Attribute(BinaryPredicate *p, Entity *e) :
    entityOpt(e)
{
    AddFirst(p);
}

AST::Attribute::Attribute(Entity *e) :
    entityOpt(e)
{
}

void AST::Attribute::SetWith(Attribute * with, HasType withType)
{
    withOpt = std::unique_ptr<Attribute>(with);
    this->withType = withType;
}

void AST::Attribute::AddFirst(BinaryPredicate * p)
{
    predicates.insert(predicates.begin(), std::unique_ptr<BinaryPredicate>(p));
}

void AST::AttributeList::Add(Attribute *a)
{
    std::unique_ptr<Attribute> ap(a);
    attributes.push_back(Attribute(std::move(*ap)));
}

CompoundName AST::AttributeList::GetCompoundName() const
{
    std::vector<int> name;
    name.reserve(attributes.size());
    for(auto &a : attributes) name.push_back(a.predicates[0]->nameId);
    return CompoundName(std::move(name));
}

AST::NewEntity::NewEntity(const SourceLocation & loc, UnaryPredicate * predicate, AttributeList * attributes) : EntityClause(loc, nullptr, new UnaryPredicateList(predicate), attributes)
{
}

AST::PragmaList::PragmaList(StringId p)
{
    Add (p);
}

void AST::PragmaList::Add(StringId p)
{
    parts.push_back(p);
}

void AST::PragmaList::Visit(Visitor&) const
{
}

void AST::Rule::SetPragma(PragmaList * p)
{
    // TODO: Merge Pragma lists
    pragmas = std::unique_ptr<PragmaList>(p);
}

void AST::Clause::SetPragma(PragmaList * p)
{
    pragmas = std::unique_ptr<PragmaList>(p);
}

AST::ArithmeticEntity::ArithmeticEntity(const SourceLocation & loc) : Entity(loc)
{
}

AST::Entity::Entity(const SourceLocation & loc) : location(loc)
{
}

SourceLocation operator+(const SourceLocation & l1, const SourceLocation & l2)
{
    assert(l1.filenameId == l2.filenameId);
    return SourceLocation { l1.filenameId, l1.line, l1.column };
}

void AST::EntityClause::AddFirst(AST::UnaryPredicate * pred)
{
    predicates->list.insert(predicates->list.begin(), std::unique_ptr<AST::UnaryPredicate>(pred));
}

void AST::EntityList::AddFirst(AST::Entity * e)
{
    entities.insert(entities.begin(), std::unique_ptr<Entity>(e));
}

AST::Attribute::Attribute(EntityClause * ec)
{
    entityClauseOpt = std::unique_ptr<EntityClause>(ec);
}

