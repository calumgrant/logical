
%require "3.4"

%code requires {
    #include <AST.hpp>
    typedef void * yyscan_t;
}

%locations
%define api.pure full
%param { yyscan_t scanner }

%parse-param { const ParseData &data };

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
    AST::Attribute* attribute;
}

%type<clause> clause andclause orclause notclause allclause datalog_predicate baseclause datalog_clause datalog_base_clause datalog_and_clause datalog_unary_clause
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
%type<attribute> attribute

%{
#include <Database.hpp>

#include "tokens.tab.h"
#include <memory>
#include <iostream>

typedef void * yyscan_t;

int yylex(YYSTYPE* yylvalp, YYLTYPE* yyllocp, yyscan_t scanner);

#define LOCATION(L1,L2) SourceLocation{data.filenameId, (L1).first_line, (L1).first_column}

void yyerror(YYLTYPE* yyllocp, yyscan_t unused, ParseData data, const char* message)
{
    auto loc = LOCATION(*yyllocp, *yyllocp);
    data.db.Error(loc, message);
}

%}

%token tok_identifier tok_atstring tok_string tok_integer tok_float tok_underscore
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_sum tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false tok_count tok_reaches tok_new
%token tok_open_square tok_close_square tok_with tok_experimental

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
        rule->Compile(data.db);
    }
|   datalog
|   tok_experimental experimental_statement
;

datalog:
    pragmaopt datalog_predicate tok_dot
    {
        std::unique_ptr<AST::Clause> clause($2);
        clause->SetPragma($1);
        clause->AssertFacts(data.db);
    }
|   pragmaopt datalog_rule tok_dot
    {
        std::unique_ptr<AST::Rule> rule($2);
        rule->SetPragma($1);
        rule->Compile(data.db);
    }
|   tok_questiondash datalog_clause tok_dot
    {
        std::unique_ptr<AST::Clause> query($2);
        query->Find(data.db);
    }
|   tok_questiondash datalog_rule tok_dot
;

datalog_predicate:
    predicate tok_open tok_close
    {
        $$ = new AST::DatalogPredicate(LOCATION(@1,@3), $1, nullptr);
    }
|   predicate tok_open entitylist tok_close
    {
        $$ = new AST::DatalogPredicate(LOCATION(@1,@4), $1, $3);
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
|   entity_expression comparator entity_expression 
    {
        $$ = new AST::Comparator(LOCATION(@1,@3), $1, $2, $3);
    }
|   entity_expression comparator entity_expression comparator entity_expression
    {
        // Technically this is too broad but anyway
        // This would allow 1>=X>=2 which we don't really want.
        $$ = new AST::Range(LOCATION(@1,@5), $1, $2, $3, $4, $5);
    }
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
|   tok_not datalog_base_clause { $$ = new AST::Not(LOCATION(@1,@1), $2); }
;

datalog_and_clause:
    datalog_unary_clause
|   datalog_and_clause tok_and datalog_unary_clause { $$ = new AST::And(LOCATION(@1,@3), $1, $3); }
|   datalog_and_clause tok_comma datalog_unary_clause { $$ = new AST::And(LOCATION(@1,@3), $1, $3); }
;

datalog_clause:
    datalog_clause tok_or datalog_and_clause
    {
        $$ = new AST::Or(LOCATION(@1, @3), $1, $3);
    }
|   datalog_clause tok_semicolon datalog_and_clause 
    {
        $$ = new AST::Or(LOCATION(@1, @3), $1, $3);
    }
|   datalog_and_clause
;

fact: 
    pragmaopt clause tok_dot 
    {
        std::unique_ptr<AST::Clause> clause($2);
        clause->SetPragma($1);
        clause->AssertFacts(data.db);
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
    entity is_a unarypredicatelist { $$ = new AST::EntityIs(LOCATION(@1, @3), $1, $3, $2); }
|   entity is_a value { $$ = new AST::NotImplementedClause(LOCATION(@1, @3), $1, $3); }
|   unarypredicatelist entity is_a unarypredicatelist { $$ = new AST::EntityIsPredicate(LOCATION(@1, @4), $2, $1, $4); }
|   entity_expression comparator entity_expression 
    {
        $$ = new AST::Comparator(LOCATION(@1, @3), $1, $2, $3);
    }
|   entity_expression comparator entity_expression comparator entity_expression
    {
        // Technically this is too broad but anyway
        // This would allow 1>=X>=2 which we don't really want.
        $$ = new AST::Range(LOCATION(@1, @5), $1, $2, $3, $4, $5);
    }
|   unarypredicatelist entity { $$ = new AST::EntityIs(LOCATION(@1, @2), $2, $1, IsType::is); }
|   entity has_a binarypredicate { $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, new AST::AttributeList($3, nullptr), $2); }
|   unarypredicatelist entity has_a attributes
    { 
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @4), $1, $2, $4, $3);
    }
|   unarypredicatelist entity reaches binarypredicate entity_expression
    { 
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @5), $1, $2, new AST::AttributeList($4,$5), $3);
    }
|   entity has_a attributes
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, $3, $2);
    }
|   entity reaches binarypredicate entity_expression
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @4), nullptr, $1, new AST::AttributeList($3,$4), $2);
    }
|   tok_open clause tok_close { $$=$2; }
|   tok_new unarypredicate tok_has attributes
    {
        $$ = new AST::NewEntity(LOCATION(@1, @4), $2, $4);
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
|   tok_comma { $$ = HasType::comma; }
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
|   tok_not allclause { $$ = new AST::Not(LOCATION(@1, @1), $2); }
;

andclause:
    notclause
|   andclause tok_and notclause { $$ = new AST::And(LOCATION(@1, @3), $1, $3); }
;

orclause:
    andclause
|   orclause tok_or andclause { $$ = new AST::Or(LOCATION(@1, @3), $1, $3); }
;

clause: orclause;

// Example: person x has name y, surname z

attribute: binarypredicate entity_expression { $$ = new AST::Attribute($1,$2); }

attributes:
    attribute { $$ = new AST::AttributeList($1); }
|   attributes tok_comma attribute
    {
        $1->Add($3);
        $$ = $1;
    }
;

predicate: tok_identifier { $$ = new AST::Predicate($1); }
unarypredicate: tok_identifier { $$ = new AST::UnaryPredicate($1); }
binarypredicate: tok_identifier { $$ = new AST::BinaryPredicate($1); } | tok_string { $$ = new AST::BinaryPredicate($1); }

variable:
    tok_identifier { $$ = new AST::NamedVariable(LOCATION(@1, @1), $1); }
|   tok_underscore { $$ = new AST::UnnamedVariable(LOCATION(@1, @1)); }
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
|   tok_minus baseentity { $$ = new AST::NegateEntity(LOCATION(@1, @2), $2); }
;

mulentity:
    unaryentity
|   mulentity tok_times unaryentity { $$ = new AST::MulEntity(LOCATION(@1, @3), $1,$3); }
|   mulentity tok_div unaryentity { $$ = new AST::DivEntity(LOCATION(@1, @3), $1,$3); }
|   mulentity tok_mod unaryentity { $$ = new AST::ModEntity(LOCATION(@1, @3), $1,$3); }
;

plusentity:
    mulentity
|   plusentity tok_plus mulentity { $$ = new AST::AddEntity(LOCATION(@1, @3), $1,$3); }
|   plusentity tok_minus mulentity { $$ = new AST::SubEntity(LOCATION(@1, @3), $1,$3); }
;

sumentity:
    plusentity
|   tok_find tok_sum variable tok_identifier variable tok_in tok_open clause tok_close
    {
        // A contextual keyword, where tok_identifier should be "over".
        $$ = new AST::Sum(LOCATION(@1, @8), $5, $3, $8);
        if ($4 != data.db.GetStringId("over"))
            yyerror(&yylloc, scanner, data, "Expecting 'over'");
    }
|   tok_find tok_sum variable tok_in tok_open clause tok_close
    {
        $$ = new AST::Sum(LOCATION(@1, @6), nullptr, $3, $6);
    }
|   tok_find tok_count entity_expression tok_in tok_open clause tok_close { $$ = new AST::Count(LOCATION(@1, @6), $3, $6); }
|   tok_find tok_count tok_open entity_expression tok_comma datalog_clause tok_close { $$ = new AST::Count(LOCATION(@1, @6), $4, $6); }
|   tok_find tok_sum tok_open variable tok_comma datalog_clause tok_close { $$ = new AST::Sum(LOCATION(@1, @6), nullptr, $4, $6); }
|   tok_find tok_sum tok_open variable tok_comma variable tok_comma datalog_clause tok_close { $$ = new AST::Sum(LOCATION(@1, @8), $4, $6, $8); }
;

entity_expression: sumentity;

value: 
    tok_string
    {
        $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::String, $1));
    }
|   tok_atstring { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::AtString, $1)); }
|   tok_integer { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Integer, $1)); }
|   tok_float   { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Float, $1)); }
|   tok_true    { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Boolean, 1)); }
|   tok_false   { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Boolean, 0)); }
;

pragmaopt: { $$ = nullptr; } | pragma;

pragma: tok_open_square pragma_list tok_close_square { $$=$2; }

pragma_list:
    tok_identifier { $$ = new AST::PragmaList($1); }
|   pragma_list tok_comma tok_identifier { $$ = $1; $$->Add($3); }
;

experimental_statement:
    experimental_clause tok_dot
|   tok_if experimental_clause tok_then experimental_clause tok_dot
|   experimental_clause tok_if experimental_clause tok_dot
|   experimental_base_clause tok_colondash experimental_clause tok_dot
|   tok_questiondash experimental_clause tok_dot
;

experimental_entity_base0:
    tok_identifier experimental_entity
|   tok_identifier tok_open experimental_entity_expression tok_close
|   tok_string experimental_entity
|   tok_string tok_open experimental_entity_expression tok_close
|   tok_identifier experimental_entity_base0
;

experimental_entity_base:
    tok_string experimental_entity
|   tok_identifier experimental_entity
|   tok_identifier experimental_entity_base0
|   tok_identifier tok_open experimental_entity_expression tok_close
;

experimental_entity_clause:
    experimental_entity_base
|   experimental_entity_base has_a experimental_attribute_list
|   experimental_entity has_a experimental_attribute_list
;

experimental_attribute_list:
    experimental_attribute
|   experimental_attribute_list tok_comma experimental_attribute
;

experimental_base_clause:
    tok_open experimental_clause tok_close
|   experimental_entity_clause
|   pragma experimental_base_clause
|   tok_not experimental_base_clause
|   experimental_entity is_a tok_identifier
|   experimental_entity comparator experimental_entity_expression
|   experimental_entity comparator experimental_entity_expression comparator experimental_entity_expression
|   tok_identifier tok_open tok_close
|   tok_identifier tok_open experimental_entity_expression tok_comma experimental_entity_expression_list tok_close
;

experimental_entity0:
    tok_identifier
|   tok_string
|   tok_atstring
|   tok_integer
|   tok_true
|   tok_false
|   tok_float
|   tok_find tok_identifier experimental_entity_expression_list tok_in tok_open experimental_clause tok_close
;

experimental_entity_expression0:
    experimental_entity0
|   tok_open experimental_entity_expression tok_close
|   tok_plus experimental_entity_expression0
|   tok_minus experimental_entity_expression0
;

experimental_entity_expression1:
    experimental_entity_expression0
|   experimental_entity_expression1 tok_times experimental_entity_expression0
|   experimental_entity_expression1 tok_div experimental_entity_expression0
|   experimental_entity_expression1 tok_mod experimental_entity_expression0
;

experimental_entity1:
    experimental_entity0
|   experimental_entity1 tok_times experimental_entity_expression0
|   experimental_entity1 tok_div experimental_entity_expression0
|   experimental_entity1 tok_mod experimental_entity_expression0
;

// A simple entity such as number or simple arithmetic expressions
experimental_entity:
    experimental_entity1
|   experimental_entity tok_plus experimental_entity_expression1
|   experimental_entity tok_minus experimental_entity_expression1
;

experimental_entity_expression:
    experimental_entity_expression1
|   experimental_entity_expression tok_plus experimental_entity_expression1
|   experimental_entity_expression tok_minus experimental_entity_expression1
;

 experimental_entity_expression_list:
    experimental_entity_expression
|   experimental_entity_expression_list tok_comma experimental_entity_expression
;

experimental_attribute:
    experimental_binpred experimental_entity
|   experimental_binpred tok_open experimental_entity_expression tok_close
|   experimental_binpred experimental_attribute
;

experimental_binpred: tok_identifier | tok_string;

experimental_and_clause:
    experimental_base_clause
|   experimental_and_clause tok_and experimental_base_clause
;

experimental_or_clause:
    experimental_and_clause
|   experimental_or_clause tok_or experimental_and_clause
;

experimental_clause:
    experimental_or_clause
;

%%
