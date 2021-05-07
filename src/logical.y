
%require "3.4"

%code requires {
    #include <AST.hpp>
}

%locations
// %pure-parser
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
    AST::AttributeList* attributes;
    int ival;
    char *sval;
    float fval;
    AST::Rule * rule;
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
%type<sval> tok_identifier tok_atstring tok_string
%type<ival> tok_integer
%type<fval> tok_float
%type<rule> datalog_rule rule

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
    {
        std::unique_ptr<AST::Rule> rule($1);
        rule->Compile(db);
    }
|   datalog
|   query
;

datalog:
    datalog_predicate tok_dot
    {
        std::unique_ptr<AST::Clause> clause($1);
        clause->AssertFacts(db);
    }
|   datalog_rule tok_dot
    {
        std::unique_ptr<AST::Rule> rule($1);
        rule->Compile(db);
    }
|   tok_questiondash datalog_predicate tok_dot
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
        db.Find(predicate->name);
    }
|   tok_find queryclause tok_if clause tok_dot
|   tok_find variablelist tok_in clause tok_dot
;

// Different syntax to distinguish them from variable lists A, B, C
querybaseclause:
    unarypredicatelist entity 
    {
        $$ = new AST::EntityIs($2, $1);
    }
|   unarypredicatelist entity has_a binarypredicate entity
    {
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4, $5, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate entity attributes
    {
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4, $5, $6));
    }
|   unarypredicatelist entity attributes
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $3);
    }
|   entity has_a binarypredicate entity
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, nullptr));
    }
|   entity has_a binarypredicate entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, $5));
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
    clause tok_dot 
    {
        std::unique_ptr<AST::Clause> clause($1);
        clause->AssertFacts(db);
    }
;

rule:
    tok_if clause tok_then clause tok_dot
    {
        $$ = new AST::Rule($4,$2);
    }
|   clause tok_if clause tok_dot
    {
        $$ = new AST::Rule($1,$3);
    }
;

baseclause:
    entity is_a unarypredicatelist { $$ = new AST::EntityIs($1, $3); }
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
|   unarypredicatelist entity { $$ = new AST::EntityIs($2, $1); }
|   entity has_a binarypredicate { $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr, nullptr)); }
|   entity tok_comma binarypredicate
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, nullptr, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate entity_expression
    { 
        $$ = new AST::EntityHasAttributes($1, $2,
            new AST::AttributeList($4, $5, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate entity_expression tok_with withlist
    {
        $$ = new AST::NotImplementedClause();
    }
|   unarypredicatelist entity has_a binarypredicate entity_expression attributes
    { 
        $$ = new AST::EntityHasAttributes($1, $2, new AST::AttributeList($4, $5, $6));
    }
|   unarypredicatelist entity attributes 
    { 
        $$ = new AST::EntityHasAttributes($1, $2, $3);
    }
|   entity has_a binarypredicate entity_expression
    { 
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, nullptr));
    }
|   entity has_a binarypredicate entity_expression tok_with withlist
    { 
        $$ = new AST::NotImplementedClause();
    }
|   entity is_a unarypredicate tok_with withlist
    { 
        $$ = new AST::NotImplementedClause();
    }
|   entity has_a binarypredicate entity_expression attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, new AST::AttributeList($3, $4, $5));
    }
|   entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, $1, $2);
    }
|   tok_open clause tok_close { $$=$2; }
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

allclause:
    baseclause
|   tok_all tok_open clause tok_close tok_in allclause 
    {
        $$ = new AST::All($6, $3);
    }
|   tok_all tok_open clause tok_close tok_then allclause
    {
        $$ = new AST::All($3, $6);
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
    tok_comma binarypredicate entity_expression { $$ = new AST::AttributeList($2, $3, nullptr); }
|   attributes tok_comma binarypredicate entity_expression { $$ = new AST::AttributeList($3, $4, $1); }
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
|   tok_sum entity_expression tok_in tok_open clause tok_close { $$ = new AST::NotImplementedEntity($2,$5); }
|   tok_count entity_expression tok_in tok_open clause tok_close { $$ = new AST::Count($2, $5); }
;

entity_expression: sumentity;

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
