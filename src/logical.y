
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
    AST::EntityClause * entityClause;
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

%type<comparator> comparator
%type<ival> tok_integer tok_identifier tok_atstring tok_string
%type<fval> tok_float
%type<is> is_a
%type<pragmas> pragma pragma_list

%type<binarypredicate> experimental_binpred
%type<entity> experimental_entity0 experimental_entity_expression0 experimental_entity_expression1 experimental_entity1 experimental_entity experimental_entity_expression experimental_literal experimental_variable
%type<entityClause> experimental_entity_base0 experimental_entity_base experimental_entity_clause
%type<attributes> experimental_attributes
%type<attribute> experimental_attribute experimental_with_attribute experimental_attribute0 experimental_attribute1
%type<unarypredicatelist> experimental_predicate_list
%type<clause> experimental_clause experimental_or_clause experimental_and_clause experimental_base_clause experimental_datalog_and_clause experimental_datalog_or_clause experimental_datalog_base_clause experimental_datalog_clause
%type<entities> experimental_entity_expression_list

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
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false tok_reaches tok_new
%token tok_open_square tok_close_square tok_with

%%

statementsopt: 
|   statements
;

statements:
    statement
|   statements statement
;

statement:
    experimental_statement
;

comparator:
    tok_equals { $$ = ComparatorType::eq; }
|   tok_notequals { $$ = ComparatorType::neq; }
|   tok_lt { $$ = ComparatorType::lt; }
|   tok_gt { $$ = ComparatorType::gt; }
|   tok_lteq { $$ = ComparatorType::lteq; }
|   tok_gteq { $$ = ComparatorType::gteq; }
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

pragma: tok_open_square pragma_list tok_close_square { $$=$2; }

pragma_list:
    tok_identifier { $$ = new AST::PragmaList($1); }
|   pragma_list tok_comma tok_identifier { $$ = $1; $$->Add($3); }
;

experimental_statement:
    experimental_clause tok_dot
    {
        std::unique_ptr<AST::Clause> clause($1);
        clause->AssertFacts(data.db);
    }
|   tok_if experimental_clause tok_then experimental_clause tok_dot
    {
        std::unique_ptr<AST::Rule> rule(new AST::Rule($4, $2));
        rule->Compile(data.db);
    }
|   experimental_clause tok_if experimental_clause tok_dot
    {
        std::unique_ptr<AST::Rule> rule(new AST::Rule($1, $3));
        rule->Compile(data.db);
    }
|   experimental_base_clause tok_colondash experimental_datalog_clause tok_dot
    {
        std::unique_ptr<AST::Rule> rule(new AST::Rule($1, $3));
        rule->Compile(data.db);
    }
|   tok_questiondash experimental_clause tok_dot
    {
        std::unique_ptr<AST::Clause> query($2);
        query->Find(data.db);
    }
|   error tok_dot
;

experimental_entity_base0:
    tok_identifier experimental_entity
    {
        $$ = new AST::EntityIs(LOCATION(@1, @2), $2, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_identifier tok_open experimental_entity_expression tok_close
    {
        $$ = new AST::EntityIs(LOCATION(@1, @4), $3, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_string experimental_entity
    {
        $$ = new AST::EntityIs(LOCATION(@1, @2), $2, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_string tok_open experimental_entity_expression tok_close
    {
        $$ = new AST::EntityIs(LOCATION(@1, @4), $3, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_identifier experimental_entity_base0
    {
        $$=$2; $$->AddFirst(new AST::UnaryPredicate($1));
    }
;

experimental_entity_base:
    tok_string experimental_entity
    {
        $$ = new AST::EntityIs(LOCATION(@1, @2), $2, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_identifier experimental_entity
    {
        $$ = new AST::EntityIs(LOCATION(@1, @2), $2, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_identifier experimental_entity_base0
    {
        $$=$2;
        $$->AddFirst(new AST::UnaryPredicate($1));
    }
|   tok_identifier tok_open experimental_entity_expression tok_close
    {
        $$ = new AST::EntityIs(LOCATION(@1, @4), $3, new AST::UnaryPredicateList(new AST::UnaryPredicate($1)), IsType::is);
    }
|   tok_identifier tok_open experimental_entity_base tok_close
    {
        $$ = $3;
        $3->AddFirst(new AST::UnaryPredicate($1));
    }
;

experimental_entity_clause:
    experimental_entity_base
    {
        $$=$1;
    }
|   experimental_entity_base tok_has experimental_attributes
    {
        $$ = $1;
        $1->SetAttributes($3, HasType::has);
    }
|   experimental_entity tok_has experimental_attributes
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, $3, HasType::has);
    }
|   experimental_entity tok_has tok_no experimental_attribute
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @4), nullptr, $1, new AST::AttributeList($4), HasType::hasnot);
    }
|   experimental_entity_base tok_comma experimental_attributes
    {
        $$ = $1;
        $1->SetAttributes($3, HasType::comma);
    }
|   experimental_entity tok_comma experimental_attributes
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, $3, HasType::comma);
    }
|   experimental_entity_base tok_reaches experimental_attribute
    {
        $$ = $1;
        $1->SetAttributes(new AST::AttributeList($3), HasType::reaches);
    }
|   experimental_entity tok_reaches experimental_attribute
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, new AST::AttributeList($3), HasType::reaches);
    }
|   experimental_entity_base tok_reaches experimental_with_attribute
    {
        $$ = $1;
        $1->SetAttributes(new AST::AttributeList($3), HasType::reaches);
    }
|   experimental_entity tok_reaches experimental_with_attribute
    {
        $$ = new AST::EntityHasAttributes(LOCATION(@1, @3), nullptr, $1, new AST::AttributeList($3), HasType::reaches);
    }
;

experimental_with_attribute:
    experimental_attribute tok_with tok_no experimental_binpred
    {
        $$ = $1;
        $$->SetWith(new AST::Attribute($4, nullptr), HasType::hasnot);
    }
|   experimental_attribute tok_with experimental_attribute
    {
        $$ = $1;
        $$->SetWith($3, HasType::has);
    }
|   experimental_attribute tok_with experimental_with_attribute
    {
        $$ = $1;
        $$->SetWith($3, HasType::has);
    }
;

experimental_attributes:
    experimental_attribute1
    {
        $$ = new AST::AttributeList($1);
    }
|   experimental_attributes tok_comma experimental_attribute1
    {
        $$ = $1;
        $$->Add($3);
    }
;

experimental_attribute0:
    experimental_entity
    {
        $$ = new AST::Attribute($1);
    }
|   tok_open experimental_entity_expression tok_close
    {
        $$ = new AST::Attribute($2);
    }
|   tok_open experimental_entity_base tok_close
    {
        $$ = new AST::Attribute($2);
    }
|   experimental_binpred experimental_attribute0
    {
        $$ = $2;
        $$->AddFirst($1);
    }
;

experimental_attribute1:
    experimental_attribute
|   experimental_with_attribute
;

experimental_attribute:
    a_opt experimental_binpred
    {
        $$ = new AST::Attribute($2, nullptr);
    }
|   a_opt experimental_binpred experimental_attribute0
    {
        $$ = $3;
        $$->AddFirst($2);
    }
;

a_opt:
|   tok_a
|   tok_an
;

experimental_binpred:
    tok_identifier
    {
        $$ = new AST::BinaryPredicate($1);
    }
|   tok_string
    {
        $$ = new AST::BinaryPredicate($1);
    }
;

experimental_base_clause:
    tok_open experimental_clause tok_close
    {
        $$= $2;
    }
|   experimental_entity_clause
    {
        $$ = $1;
    }
|   pragma experimental_base_clause
    {
        $$ = $2;
        $$->SetPragma($1);
    }
|   tok_not experimental_base_clause
    {
        $$ = new AST::Not(LOCATION(@1,@2), $2);
    }
|   experimental_entity is_a experimental_predicate_list
    {
        $$ = new AST::EntityIs(LOCATION(@1, @3), $1, $3, $2);
    }
|   experimental_entity comparator experimental_entity_expression
    {
        $$ = new AST::Comparator(LOCATION(@1, @3), $1, $2, $3);
    }
|   experimental_entity comparator experimental_entity_expression comparator experimental_entity_expression
    {
        // Technically this is too broad but anyway
        // This would allow 1>=X>=2 which we don't really want.
        $$ = new AST::Range(LOCATION(@1, @5), $1, $2, $3, $4, $5);
    }
|   tok_identifier tok_open tok_close
    {
        $$ = new AST::DatalogPredicate(LOCATION(@1,@3), new AST::Predicate($1), nullptr);
    }
|   tok_identifier tok_open experimental_entity_expression tok_comma experimental_entity_expression_list tok_close
    {
        $5->AddFirst($3);
        $$ = new AST::DatalogPredicate(LOCATION(@1,@6), new AST::Predicate($1), $5);
    }
|   tok_identifier
    {
        // Nonary clause
        $$ = new AST::DatalogPredicate(LOCATION(@1,@1), new AST::Predicate($1), nullptr);
    }  
|   tok_all tok_open experimental_clause tok_close tok_in tok_open experimental_clause tok_close
    {
        $$ = AST::MakeAll($7, $3);
    }
;

experimental_predicate_list:
    tok_identifier
    {
        $$ = new AST::UnaryPredicateList(new AST::UnaryPredicate($1));
    }
|   experimental_predicate_list tok_identifier
    {
        $$ = $1; $$->Append(new AST::UnaryPredicate($2));
    }
;

experimental_datalog_base_clause:
    tok_open experimental_datalog_clause tok_close
    {
        $$ = $2;
    }
|   pragma experimental_datalog_base_clause
    {
        $$ = $2;
        $$->SetPragma($1);
    }
|   tok_not experimental_datalog_base_clause
    {
        $$ = new AST::Not(LOCATION(@1, @1), $2);
    }
|   experimental_entity_expression comparator experimental_entity_expression
    {
        $$ = new AST::Comparator(LOCATION(@1,@3), $1, $2, $3);
    }
|   experimental_entity_expression comparator experimental_entity_expression comparator experimental_entity_expression
    {
        // Technically this is too broad but anyway
        // This would allow 1>=X>=2 which we don't really want.
        $$ = new AST::Range(LOCATION(@1,@5), $1, $2, $3, $4, $5);
    }
|   tok_identifier tok_open tok_close
    {
        $$ = new AST::DatalogPredicate(LOCATION(@1,@3), new AST::Predicate($1), nullptr);
    }
|   tok_identifier tok_open experimental_entity_expression_list tok_close
    {
        $$ = new AST::DatalogPredicate(LOCATION(@1,@4), new AST::Predicate($1), $3);
    }
|   tok_all tok_open experimental_datalog_base_clause tok_semicolon experimental_datalog_clause tok_close
    {
        $$ = AST::MakeAll($5, $3);
    }
;

experimental_literal:
    tok_string   { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::String, $1)); }
|   tok_atstring { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::AtString, $1)); }
|   tok_integer  { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Integer, $1)); }
|   tok_true     { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Boolean, 1)); }
|   tok_false    { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Boolean, 0)); }
|   tok_float    { $$ = new AST::Value(LOCATION(@1, @1), Entity(EntityType::Float, $1)); }
;

// The keywords "a", "an" and "no" can be used as variables
experimental_variable:
    tok_identifier { $$ = new AST::NamedVariable(LOCATION(@1, @1), $1); }
|   tok_underscore { $$ = new AST::UnnamedVariable(LOCATION(@1, @1)); }
|   tok_a  { $$ = new AST::NamedVariable(LOCATION(@1, @1), data.db.GetStringId("a")); }
|   tok_an { $$ = new AST::NamedVariable(LOCATION(@1, @1), data.db.GetStringId("an")); }
|   tok_no { $$ = new AST::NamedVariable(LOCATION(@1, @1), data.db.GetStringId("no")); }
;

experimental_entity0:
    experimental_variable
|   experimental_literal
|   tok_find tok_identifier experimental_entity_expression_list tok_in tok_open experimental_clause tok_close
    {
        if( $2 == data.db.GetStringId("sum"))
            $$ = new AST::Sum(LOCATION(@1, @7), $3, $6);
        else if( $2 == data.db.GetStringId("count"))
            $$ = new AST::Count(LOCATION(@1, @7), $3, $6);
        else
            yyerror(&@2, scanner, data, "Unrecognised quantifier");
    }
|   tok_find tok_identifier tok_open experimental_entity_expression_list tok_semicolon experimental_datalog_clause tok_close
    {
        if( $2 == data.db.GetStringId("sum"))
            $$ = new AST::Sum(LOCATION(@1, @7), $4, $6);
        else if( $2 == data.db.GetStringId("count"))
            $$ = new AST::Count(LOCATION(@1, @7), $4, $6);
        else
            yyerror(&@2, scanner, data, "Unrecognised quantifier");
    }
;

experimental_entity_expression0:
    experimental_entity0
|   tok_open experimental_entity_expression tok_close { $$=$2; }
|   tok_minus experimental_entity_expression0 { $$ = new AST::NegateEntity(LOCATION(@1, @2), $2); }
;

experimental_entity_expression1:
    experimental_entity_expression0
|   experimental_entity_expression1 tok_times experimental_entity_expression0 { $$ = new AST::MulEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity_expression1 tok_div experimental_entity_expression0   { $$ = new AST::DivEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity_expression1 tok_mod experimental_entity_expression0   { $$ = new AST::ModEntity(LOCATION(@1, @3), $1,$3); }
;

experimental_entity1:
    experimental_entity0
|   experimental_entity1 tok_times experimental_entity_expression0 { $$ = new AST::MulEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity1 tok_div experimental_entity_expression0 { $$ = new AST::DivEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity1 tok_mod experimental_entity_expression0 { $$ = new AST::ModEntity(LOCATION(@1, @3), $1,$3); }
;

// A simple entity such as number or simple arithmetic expressions
experimental_entity:
    experimental_entity1
|   experimental_entity tok_plus experimental_entity_expression1  { $$ = new AST::AddEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity tok_minus experimental_entity_expression1 { $$ = new AST::SubEntity(LOCATION(@1, @3), $1,$3); }
;

experimental_entity_expression:
    experimental_entity_expression1
|   experimental_entity_expression tok_plus experimental_entity_expression1  { $$ = new AST::AddEntity(LOCATION(@1, @3), $1,$3); }
|   experimental_entity_expression tok_minus experimental_entity_expression1 { $$ = new AST::SubEntity(LOCATION(@1, @3), $1,$3); }
;

 experimental_entity_expression_list:
    experimental_entity_expression
    {
        $$ = new AST::EntityList($1);
    }
|   experimental_entity_expression_list tok_comma experimental_entity_expression
    {
        $1->Add($3);
        $$ = $1;
    }
;

experimental_and_clause:
    experimental_base_clause
|   experimental_and_clause tok_and experimental_base_clause
    {
        $$ = new AST::And(LOCATION(@1,@3), $1, $3);
    }
;

experimental_datalog_and_clause:
    experimental_datalog_base_clause
|   experimental_datalog_and_clause tok_and experimental_datalog_base_clause
    {
        $$ = new AST::And(LOCATION(@1,@3), $1, $3);
    }
|   experimental_datalog_and_clause tok_comma experimental_datalog_base_clause
    {
        $$ = new AST::And(LOCATION(@1,@3), $1, $3);
    }
;

experimental_or_clause:
    experimental_and_clause
|   experimental_or_clause tok_or experimental_and_clause
    {
        $$ = new AST::Or(LOCATION(@1,@3), $1, $3);
    }
;

experimental_datalog_or_clause:
    experimental_datalog_and_clause
|   experimental_datalog_or_clause tok_or experimental_datalog_and_clause
    {
        $$ = new AST::Or(LOCATION(@1,@3), $1, $3);
    }
|   experimental_datalog_or_clause tok_semicolon experimental_datalog_and_clause
    {
        $$ = new AST::Or(LOCATION(@1,@3), $1, $3);
    }
;

experimental_clause:
    experimental_or_clause
;    

// The difference between Datalog clauses and regular clauses is that commas
// mean "and"
experimental_datalog_clause:
    experimental_datalog_or_clause
;

%%
