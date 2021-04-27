
%require "3.4"

%code requires {
    #include <AST.hpp>

    enum class ComparatorType { lt, lteq, gt, gteq, eq, neq };
}

%locations
// %pure-parser
%parse-param { Database &db };

%union
{
    AST::Node *node;
    AST::Term* term;
    AST::EntityList* entities;
    AST::UnaryPredicateList* unarypredicatelist;
    AST::Predicate* predicate;
    AST::Entity* entity;
    AST::UnaryPredicate* unarypredicate;
    AST::BinaryPredicate* binarypredicate;
    ComparatorType comparator;
    AST::AttributeList* attributes;
    int ival;
    char *sval;
    float fval;
}

%type<term> term andterm orterm notterm allterm datalog_predicate baseterm datalog_term datalog_base_term datalog_and_term datalog_unary_term
%type<entities> entitylist
%type<unarypredicatelist> unarypredicatelist
%type<entity> entity arithmetic_entity value variable baseentity sumentity plusentity mulentity unaryentity
%type<predicate> predicate
%type<unarypredicate> unarypredicate
%type<binarypredicate> binarypredicate
%type<attributes> attributes
%type<comparator> comparator
%type<sval> tok_identifier tok_atstring tok_string
%type<ival> tok_integer
%type<fval> tok_float

%{
#include <Database.hpp>

#include "tokens.tab.h"
#include <memory>
#include <iostream>

int yylex();
extern int yylineno;
extern int yyleng;
void yyerror(Database &db, const char*message)
{
    std::cerr << message << " at line " << yylineno << std::endl;
}

%}

%token tok_identifier tok_atstring tok_string tok_integer tok_float tok_underscore
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_sum tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false tok_count tok_with

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
|   datalog
|   query
;

datalog:
    datalog_predicate tok_dot
    {
        std::unique_ptr<AST::Term> term($1);
        term->AssertFacts(db);
    }
|   datalog_rule tok_dot
|   tok_questiondash datalog_predicate tok_dot
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
    arithmetic_entity { $$ = new AST::EntityList($1); }
|   entitylist tok_comma arithmetic_entity { $1->Add($3); }
;

datalog_rule:
    datalog_predicate tok_colondash datalog_term
    {
        std::unique_ptr<AST::Term> lhs($1);
        std::unique_ptr<AST::Term> rhs($3);
        lhs->AssertRule(db, *rhs);
    }
;

datalog_base_term:
    datalog_predicate
|   entity comparator entity { $$ = new AST::NotImplementedTerm($1, $3); }
|   tok_open datalog_term tok_close { $$ = $2; }
;

comparator:
    tok_equals { $$ = ComparatorType::eq; }
|   tok_notequals { $$ = ComparatorType::neq; }
|   tok_lt { $$ = ComparatorType::lt; }
|   tok_gt { $$ = ComparatorType::gt; }
|   tok_lteq { $$ = ComparatorType::lteq; }
|   tok_gteq { $$ = ComparatorType::gteq; }
;

datalog_unary_term:
    datalog_base_term
|   tok_not datalog_base_term { $$ = new AST::NotImplementedTerm($2); }
;

datalog_and_term:
    datalog_unary_term
|   datalog_and_term tok_and datalog_unary_term
|   datalog_and_term tok_comma datalog_unary_term
;

datalog_term:
    datalog_term tok_or datalog_and_term
    {
        $$ = new AST::NotImplementedTerm($1, $3);
    }
|   datalog_term tok_semicolon datalog_and_term 
    {
        $$ = new AST::NotImplementedTerm($1, $3);
    }
|   datalog_and_term
;

query:
    tok_find queryterm tok_dot
|   tok_find predicate tok_dot
|   tok_find queryterm tok_if term tok_dot
|   tok_find variablelist tok_if term tok_dot  // find A, surname S could be am attribute or a variable list 
;

// Different syntax to distinguish them from variable lists A, B, C
querybaseterm:
    unarypredicate variable
|   unarypredicate variable has_a binarypredicate variable
|   unarypredicate variable has_a binarypredicate variable attributes
|   unarypredicate variable attributes
|   variable has_a binarypredicate variable
|   variable has_a binarypredicate variable attributes
;

queryterm:
    querybaseterm
|   queryterm tok_and querybaseterm
;

variablelist:
    variable
|   variablelist tok_comma variable
;

fact: 
    term tok_dot 
    {
        std::unique_ptr<AST::Term> term($1);
        term->AssertFacts(db);
    }
;

rule:
    tok_if term tok_then term tok_dot
    {
        std::unique_ptr<AST::Term> lhs($4);
        std::unique_ptr<AST::Term> rhs($2);
        lhs->AssertRule(db, *rhs);
    }
|   term tok_if term tok_dot
    {
        std::unique_ptr<AST::Term> lhs($1);
        std::unique_ptr<AST::Term> rhs($3);
        lhs->AssertRule(db, *rhs);
    }
;

baseterm:
    entity is_a unarypredicate { $$ = new AST::TermIs($1, $3); }
|   entity is_a value { $$ = new AST::NotImplementedTerm($1, $3); }
|   unarypredicatelist entity is_a unarypredicate { $$ = new AST::TermIsPredicate($2, $1, $4); }
|   arithmetic_entity comparator arithmetic_entity { $$ = new AST::NotImplementedTerm($1, $3); }
|   unarypredicatelist entity { $$ = new AST::TermIs($2, $1); }
|   entity has_a binarypredicate { $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr, nullptr)); }
|   entity tok_comma binarypredicate
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity
    { 
        $$ = new AST::EntityHasAttributes($1, $2, 
            new AST::AttributeList($4, $5, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity tok_with withlist
    {
        $$ = new AST::NotImplementedTerm();
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity attributes
    { 
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4, $5, $6));
    }
|   unarypredicatelist entity attributes 
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $3);
    }
|   entity has_a binarypredicate arithmetic_entity
    { 
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, nullptr));
    }
|   entity has_a binarypredicate arithmetic_entity tok_with withlist
    { 
        $$ = new AST::NotImplementedTerm();
    }
|   entity is_a unarypredicate tok_with withlist
    { 
        $$ = new AST::NotImplementedTerm();
    }
|   entity has_a binarypredicate arithmetic_entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, $5));
    }
|   entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, $2);
    }
|   tok_open term tok_close { $$=$2; }
;

unarypredicatelist:
    unarypredicate { $$ = new AST::UnaryPredicateList($1); }
|   unarypredicatelist unarypredicate { $$=$1; $$->Append($2); }
;

withlist:
    unarypredicate entity
|   withlist tok_comma unarypredicate entity
;

has_a:
    tok_has
|   tok_has tok_a
|   tok_has tok_an
|   tok_has tok_no
;

is_a:
    tok_is
|   tok_is tok_a
|   tok_is tok_an
|   tok_in
|   tok_is tok_not
|   tok_is tok_not tok_a
|   tok_is tok_not tok_an
;

allterm:
    baseterm
|   tok_all tok_open term tok_close tok_in allterm 
    {
        $$ = new AST::NotImplementedTerm($3, $6);
    }
;

notterm:
    allterm
|   tok_not allterm { $$ = new AST::NotImplementedTerm($2); }
;

andterm:
    notterm
|   andterm tok_and notterm { $$ = new AST::And($1, $3); }
;

orterm:
    andterm
|   orterm tok_or andterm
;

term: orterm;

// Example: person x has name y, surname z

attributes:
    tok_comma binarypredicate arithmetic_entity { $$ = new AST::AttributeList($2, $3, nullptr); }
|   attributes tok_comma binarypredicate arithmetic_entity { $$ = new AST::AttributeList($3, $4, $1); }
;

predicate: tok_identifier { $$ = new AST::Predicate($1); free($1); }
unarypredicate: tok_identifier { $$ = new AST::UnaryPredicate($1); free($1); }
binarypredicate: tok_identifier { $$ = new AST::BinaryPredicate($1); free($1); }

variable:
    tok_identifier { $$ = new AST::NamedVariable($1); free($1); }
|   tok_underscore { $$ = new AST::UnnamedVariable(); }
;

entity:
    value
|   variable

baseentity:
    entity
|   tok_open arithmetic_entity tok_close { $$ = $2; }
;

unaryentity:
    baseentity
|   tok_minus baseentity { $$ = new AST::NotImplementedEntity($2); }
;

mulentity:
    unaryentity
|   mulentity tok_times unaryentity { $$ = new AST::NotImplementedEntity($1,$3); }
|   mulentity tok_div unaryentity { $$ = new AST::NotImplementedEntity($1,$3); }
|   mulentity tok_mod unaryentity { $$ = new AST::NotImplementedEntity($1,$3); }
;

plusentity:
    mulentity
|   plusentity tok_plus mulentity { $$ = new AST::NotImplementedEntity($1,$3); }
|   plusentity tok_minus mulentity { $$ = new AST::NotImplementedEntity($1,$3); }
;

sumentity:
    plusentity
|   tok_sum arithmetic_entity tok_in tok_open term tok_close { $$ = new AST::NotImplementedEntity($2,$5); }
|   tok_count variable tok_in tok_open term tok_close { $$ = new AST::NotImplementedEntity($2,$5); }
;

arithmetic_entity: sumentity;

value: 
    tok_string { 
        std::string value;
        for(int i=1; i<yyleng-1; ++i)
        {
            if($1[i]=='\\')
            {
                ++i;
                switch($1[i])
                {
                case 'r':
                    value.push_back('\r');
                    break;
                case 'n':
                    value.push_back('\n');
                    break;
                case 't':
                    value.push_back('\t');
                    break;
                case '\\':
                    value.push_back('\\');
                    break;
                case '"':
                    value.push_back('"');
                    break;
                default:
                    yyerror(db, "Invalid escape character");
                    break;
                }
            }
            else
                value.push_back($1[i]);
        }
        free($1);
        $$ = new AST::String(value);
    }
|   tok_atstring { $$ = new AST::AtString($1+1); free($1); }
|   tok_integer { $$ = new AST::Integer($1); }
|   tok_float   { $$ = new AST::Float($1); }
|   tok_true    { $$ = new AST::Bool(true); }
|   tok_false   { $$ = new AST::Bool(false); }
;

%%