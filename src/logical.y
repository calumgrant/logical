
%require "3.4"

%code requires {
    #include <AST.hpp>
    typedef void * yyscan_t;
}

%locations
%define api.pure full
%param { yyscan_t scanner }

%parse-param { Database &db };

%union
{
    AST::Node *node;
    AST::Clause* clause;
    AST::EntityList* entities;
    AST::UnaryPredicateList* unarypredicatelist;
    AST::Predicate* predicate;
    AST::Entity* entity;
    AST::UnaryPredicate* unarypredicate;
    AST::BinaryPredicate* binarypredicate;
    ComparatorType comparator;
    IsType is;
    HasType has;
    AST::AttributeList* attributes;
    int ival;
    char *sval;
    float fval;
    AST::Rule * rule;
    AST::PragmaList* pragmas;
}

%type<clause> clause queryclause querybaseclause andclause orclause notclause allclause datalog_predicate baseclause datalog_clause datalog_base_clause datalog_and_clause datalog_unary_clause
%type<entities> entitylist
%type<unarypredicatelist> unarypredicatelist
%type<entity> entity entity_expression value variable baseentity sumentity plusentity mulentity unaryentity
%type<predicate> predicate
%type<unarypredicate> unarypredicate
%type<binarypredicate> binarypredicate
%type<attributes> attributes
%type<comparator> comparator
%type<ival> tok_integer tok_identifier tok_atstring tok_string
%type<fval> tok_float
%type<rule> datalog_rule rule
%type<is> is_a
%type<has> has_a reaches
%type<pragmas> pragma pragma_list pragmaopt

%{
#include <Database.hpp>

#include "tokens.tab.h"
#include <memory>
#include <iostream>

typedef void * yyscan_t;

  int yylex(YYSTYPE* yylvalp, YYLTYPE* yyllocp, yyscan_t scanner);
  void yyerror(YYLTYPE* yyllocp, yyscan_t unused, Database &db, const char* message)
  {
    std::cerr << message << " at line " << yyllocp->first_line << ":" << yyllocp->first_column << std::endl;
  }

%}

%token tok_identifier tok_atstring tok_string tok_integer tok_float tok_underscore
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_sum tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false tok_count tok_reaches tok_new
%token tok_open_square tok_close_square

%%

statementsopt: 
|   statements
;

statements:
    statement
|   statements statement
;

statement:
    fact
|   rule
    {
        std::unique_ptr<AST::Rule> rule($1);
        rule->Compile(db);
    }
|   datalog
|   query
;

datalog:
    pragmaopt datalog_predicate tok_dot
    {
        std::unique_ptr<AST::Clause> clause($2);
        clause->SetPragma($1);
        clause->AssertFacts(db);
    }
|   pragmaopt datalog_rule tok_dot
    {
        std::unique_ptr<AST::Rule> rule($2);
        rule->SetPragma($1);
        rule->Compile(db);
    }
|   tok_questiondash datalog_predicate tok_dot
    {
        std::unique_ptr<AST::Clause> query($2);
        query->Find(db);
    }
|   tok_questiondash datalog_rule tok_dot
;

datalog_predicate:
    predicate tok_open tok_close
    {
        $$ = new AST::DatalogPredicate($1, nullptr);
    }
|   predicate tok_open entitylist tok_close
    {
        $$ = new AST::DatalogPredicate($1, $3);
    }
;

entitylist:
    entity_expression { $$ = new AST::EntityList($1); }
|   entitylist tok_comma entity_expression { $1->Add($3); }
;

datalog_rule:
    datalog_predicate tok_colondash datalog_clause
    {
        $$ = new AST::Rule($1,$3);
    }
;

datalog_base_clause:
    datalog_predicate
|   entity comparator entity { $$ = new AST::NotImplementedClause($1, $3); }
|   tok_open datalog_clause tok_close { $$ = $2; }
;

comparator:
    tok_equals { $$ = ComparatorType::eq; }
|   tok_notequals { $$ = ComparatorType::neq; }
|   tok_lt { $$ = ComparatorType::lt; }
|   tok_gt { $$ = ComparatorType::gt; }
|   tok_lteq { $$ = ComparatorType::lteq; }
|   tok_gteq { $$ = ComparatorType::gteq; }
;

datalog_unary_clause:
    datalog_base_clause
|   tok_not datalog_base_clause { $$ = new AST::Not($2); }
;

datalog_and_clause:
    datalog_unary_clause
|   datalog_and_clause tok_and datalog_unary_clause { $$ = new AST::And($1, $3); }
|   datalog_and_clause tok_comma datalog_unary_clause { $$ = new AST::And($1, $3); }
;

datalog_clause:
    datalog_clause tok_or datalog_and_clause
    {
        $$ = new AST::Or($1, $3);
    }
|   datalog_clause tok_semicolon datalog_and_clause 
    {
        $$ = new AST::Or($1, $3);
    }
|   datalog_and_clause
;

query:
    tok_find queryclause tok_dot
    {
        std::unique_ptr<AST::Clause> query($2);
        query->Find(db);
    }
|   tok_find predicate tok_dot
    {
        std::unique_ptr<AST::Predicate> predicate($2);
        db.Find(PredicateName(1, predicate->nameId));
    }
|   tok_find queryclause tok_if clause tok_dot
|   tok_find variablelist tok_in clause tok_dot
;

// Different syntax to distinguish them from variable lists A, B, C
querybaseclause:
    unarypredicatelist entity
    {
        $$ = new AST::EntityIs($2, $1, IsType::is);
    }
|   unarypredicatelist entity has_a attributes
    {
        $$ = new AST::EntityHasAttributes($1, $2, $4, $3);
    }
|   unarypredicatelist entity reaches binarypredicate entity_expression
    {
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4,$5), $3);
    }
|   unarypredicatelist entity tok_comma attributes
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $4, HasType::has);
    }
|   entity has_a attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, $3, $2);
    }
|   entity reaches binarypredicate entity_expression
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3,$4), $2);
    }
;

queryclause:
    querybaseclause
|   queryclause tok_and querybaseclause
;

variablelist:
    variable
|   variablelist tok_comma variable
;

fact: 
    pragmaopt clause tok_dot 
    {
        std::unique_ptr<AST::Clause> clause($2);
        clause->SetPragma($1);
        clause->AssertFacts(db);
    }
;

rule:
    pragmaopt tok_if clause tok_then clause tok_dot
    {
        $$ = new AST::Rule($5,$3);
        $$->SetPragma($1);
    }
|   pragmaopt clause tok_if clause tok_dot
    {
        $$ = new AST::Rule($2,$4);
        $$->SetPragma($1);
    }
;

baseclause:
    entity is_a unarypredicatelist { $$ = new AST::EntityIs($1, $3, $2); }
|   entity is_a value { $$ = new AST::NotImplementedClause($1, $3); }
|   unarypredicatelist entity is_a unarypredicatelist { $$ = new AST::EntityIsPredicate($2, $1, $4); }
|   entity_expression comparator entity_expression 
    {
        $$ = new AST::Comparator($1, $2, $3);
    }
|   entity_expression comparator entity_expression comparator entity_expression
    {
        // Technically this is too broad but anyway
        // This would allow 1>=X>=2 which we don't really want.
        $$ = new AST::Range($1, $2, $3, $4, $5);
    }
|   unarypredicatelist entity { $$ = new AST::EntityIs($2, $1, IsType::is); }
|   entity has_a binarypredicate { $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr), $2); }
|   entity tok_comma binarypredicate
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr), HasType::has);
    }
|   unarypredicatelist entity has_a attributes
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $4, $3);
    }
|   unarypredicatelist entity reaches binarypredicate entity_expression
    { 
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4,$5), $3);
    }
|   unarypredicatelist entity tok_comma attributes 
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $4, HasType::has);
    }
|   entity has_a attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, $3, $2);
    }
|   entity reaches binarypredicate entity_expression
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3,$4), $2);
    }
|   entity tok_comma attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, $3, HasType::has);
    }
|   tok_open clause tok_close { $$=$2; }
|   tok_new unarypredicate tok_has attributes
    {
        $$ = new AST::NewEntity($2, $4);
    }
;

unarypredicatelist:
    unarypredicate { $$ = new AST::UnaryPredicateList($1); }
|   unarypredicatelist unarypredicate { $$=$1; $$->Append($2); }
;

has_a:
    tok_has { $$ = HasType::has; }
|   tok_has tok_a { $$ = HasType::has; }
|   tok_has tok_an { $$ = HasType::has; }
|   tok_has tok_no { $$ = HasType::hasnot; }
;

is_a:
    tok_is { $$ = IsType::is; }
|   tok_is tok_a { $$ = IsType::is; }
|   tok_is tok_an { $$ = IsType::is; }
|   tok_in { $$ = IsType::is; }
|   tok_is tok_not { $$ = IsType::isnot; }
|   tok_is tok_not tok_a { $$ = IsType::isnot; }
|   tok_is tok_not tok_an { $$ = IsType::isnot; }
;

reaches:
    tok_reaches { $$ = HasType::reaches; }
|   tok_reaches tok_a { $$ = HasType::reaches; }
|   tok_reaches tok_an { $$ = HasType::reaches; }
|   tok_reaches tok_no { $$ = HasType::reachesno; }
;

allclause:
    baseclause
|   tok_all tok_open clause tok_close tok_in allclause 
    {
        $$ = AST::MakeAll($6, $3);
    }
|   tok_all tok_open clause tok_close tok_then allclause
    {
        $$ = AST::MakeAll($3, $6);
    }
;

notclause:
    allclause
|   tok_not allclause { $$ = new AST::Not($2); }
;

andclause:
    notclause
|   andclause tok_and notclause { $$ = new AST::And($1, $3); }
;

orclause:
    andclause
|   orclause tok_or andclause { $$ = new AST::Or($1, $3); }
;

clause: orclause;

// Example: person x has name y, surname z

attributes:
    binarypredicate entity_expression { $$ = new AST::AttributeList($1, $2); }
|   attributes tok_comma binarypredicate entity_expression
    {
        $1->Add($3, $4);
        $$ = $1;
    }
;

predicate: tok_identifier { $$ = new AST::Predicate($1); }
unarypredicate: tok_identifier { $$ = new AST::UnaryPredicate($1); }
binarypredicate: tok_identifier { $$ = new AST::BinaryPredicate($1); }

variable:
    tok_identifier { $$ = new AST::NamedVariable($1, @1.first_line, @1.first_column); }
|   tok_underscore { $$ = new AST::UnnamedVariable(@1.first_line, @1.first_column); }
;

entity:
    value
|   variable

baseentity:
    entity
|   tok_open entity_expression tok_close { $$ = $2; }
;

unaryentity:
    baseentity
|   tok_minus baseentity { $$ = new AST::NegateEntity($2); }
;

mulentity:
    unaryentity
|   mulentity tok_times unaryentity { $$ = new AST::MulEntity($1,$3); }
|   mulentity tok_div unaryentity { $$ = new AST::DivEntity($1,$3); }
|   mulentity tok_mod unaryentity { $$ = new AST::ModEntity($1,$3); }
;

plusentity:
    mulentity
|   plusentity tok_plus mulentity { $$ = new AST::AddEntity($1,$3); }
|   plusentity tok_minus mulentity { $$ = new AST::SubEntity($1,$3); }
;

sumentity:
    plusentity
|   tok_sum variable tok_identifier variable tok_in tok_open clause tok_close
    {
        // A contextual keyword, where tok_identifier should be "over".
        $$ = new AST::Sum($4, $2, $7);
        if ($3 != db.GetStringId("over"))
            yyerror(&yylloc, scanner, db, "Expecting 'over'");
    }
|   tok_sum variable tok_in tok_open clause tok_close
    {
        $$ = new AST::Sum(nullptr, $2, $5);
    }
|   tok_count entity_expression tok_in tok_open clause tok_close { $$ = new AST::Count($2, $5); }
;

entity_expression: sumentity;

value: 
    tok_string
    {
        $$ = new AST::Value(Entity(EntityType::String, $1));
    }
|   tok_atstring { $$ = new AST::Value(Entity(EntityType::AtString, $1)); }
|   tok_integer { $$ = new AST::Value(Entity(EntityType::Integer, $1)); }
|   tok_float   { $$ = new AST::Value(Entity(EntityType::Float, $1)); }
|   tok_true    { $$ = new AST::Value(Entity(EntityType::Boolean, 1)); }
|   tok_false   { $$ = new AST::Value(Entity(EntityType::Boolean, 0)); }
;

pragmaopt: { $$ = nullptr; } | pragma;

pragma: tok_open_square pragma_list tok_close_square { $$=$2; }

pragma_list:
    tok_identifier { $$ = new AST::PragmaList($1); }
|   pragma_list tok_comma tok_identifier { $$ = $1; $$->Add($3); }
;

%%
