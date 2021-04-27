// %skeleton "lalr1.cc"

%locations
// %pure-parser


%{
#include <AST.hpp>
#define YYSTYPE AST::Node*

// This is totally wrong I think
extern char yytext[];

#include "tokens.tab.h"
#include <memory>
#include <iostream>

int yylex();
extern int yylineno;
extern int yyleng;
void yyerror(const char*) { std::cerr << "Syntax error at line " << yylineno; }

// int yylex(YYLVAL * val, YYLTYPE * loc);
//void yyerror(YYLTYPE *loc, const char*) { std::cerr << "Syntax error at ?????"; }

//    #define YYSTYPE bool
//    void yyerror (yyscan_t yyscanner, char const *msg);

// What to do
void ProcessFact(AST::Term * statement);
void ProcessRule(AST::Term * lhs, AST::Term * rhs);

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
    datalog_predicate tok_dot { ProcessFact((AST::Term*)$1); }
|   datalog_rule tok_dot
|   tok_questiondash datalog_predicate tok_dot
;

datalog_predicate:
    predicate tok_open tok_close { $$ = new AST::DatalogPredicate((AST::Predicate*)$1, nullptr); }
|   predicate tok_open entitylist tok_close { $$ = new AST::DatalogPredicate((AST::Predicate*)$1, (AST::EntityList*)$3); }
;

entitylist:
    arithmetic_entity { $$ = new AST::EntityList((AST::Entity*)$1); }
|   entitylist tok_comma arithmetic_entity { ((AST::EntityList*)$1)->Add((AST::Entity*)$3); }
;

datalog_rule:
    datalog_predicate tok_colondash datalog_term
    {
        ProcessRule((AST::Term*)$1,(AST::Term*)$3);
    }
;

datalog_base_term:
    datalog_predicate
|   entity comparator entity
|   tok_open datalog_term tok_close { $$ = $2; }
;

comparator:
    tok_equals | tok_notequals | tok_lt | tok_gt | tok_lteq | tok_gteq
    ;

datalog_unary_term:
    datalog_base_term
|   tok_not datalog_base_term
;

datalog_and_term:
    datalog_unary_term
|   datalog_and_term tok_and datalog_unary_term
|   datalog_and_term tok_comma datalog_unary_term
;

datalog_term:
|   datalog_term tok_or datalog_and_term
|   datalog_term tok_semicolon datalog_and_term
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

fact: term tok_dot { ProcessFact((AST::Term*)$1); };

rule:
    tok_if term tok_then term tok_dot { ProcessRule((AST::Term*)$4, (AST::Term*)$2); }
|   term tok_if term tok_dot { ProcessRule((AST::Term*)$1, (AST::Term*)$3); }
;

baseterm:
    entity is_a unarypredicate { $$ = new AST::TermIs((AST::Entity*)$1, (AST::UnaryPredicate*)$3); }
|   entity is_a value { $$ = new AST::NotImplementedTerm($1, $3); }
|   unarypredicatelist entity is_a unarypredicate { $$ = new AST::TermIsPredicate((AST::Entity*)$2, (AST::UnaryPredicateList*)$1, (AST::UnaryPredicate*)$4); }
|   arithmetic_entity comparator arithmetic_entity { $$ = new AST::NotImplementedTerm($1, $3); }
|   unarypredicatelist entity { $$ = new AST::TermIs((AST::Entity*)$2, (AST::UnaryPredicateList*)$1); }
|   entity has_a binarypredicate { $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, new AST::AttributeList((AST::BinaryPredicate*)$3, nullptr, nullptr)); }
|   entity tok_comma binarypredicate
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, new AST::AttributeList((AST::BinaryPredicate*)$3, nullptr, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, 
            new AST::AttributeList((AST::BinaryPredicate*)$4, (AST::Entity*)$5, nullptr));
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity tok_with inlist
    {
        $$ = new AST::NotImplementedTerm();
    }
|   unarypredicatelist entity has_a binarypredicate arithmetic_entity attributes
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, 
            new AST::AttributeList((AST::BinaryPredicate*)$4, (AST::Entity*)$5, (AST::AttributeList*)$6));
    }
|   unarypredicatelist entity attributes 
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, (AST::AttributeList*)$3);
    }
|   entity has_a binarypredicate arithmetic_entity
    { 
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, 
            new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, nullptr));
    }
|   entity has_a binarypredicate arithmetic_entity tok_with inlist
    { 
        $$ = new AST::NotImplementedTerm();
    }
|   entity has_a binarypredicate arithmetic_entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, 
            new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, (AST::AttributeList*)$5));
    }
|   entity attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, (AST::AttributeList*)$2);
    }
|   tok_open term tok_close { $$=$2; }
;

unarypredicatelist:
    unarypredicate { $$ = new AST::UnaryPredicateList((AST::UnaryPredicate*)$1); }
|   unarypredicatelist unarypredicate { $$=$1; ((AST::UnaryPredicateList*)$$)->Append((AST::UnaryPredicate*)$2); }
;

inlist:
    unarypredicate entity
|   inlist tok_comma unarypredicate entity
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
;

notterm:
    allterm
|   tok_not allterm
;

andterm:
    notterm
|   andterm tok_and notterm { $$ = new AST::And((AST::Term*)$1, (AST::Term*)$3); }
;

orterm:
    andterm
|   orterm tok_or andterm
;

term: orterm;

// Example: person x has name y, surname z

attributes:
    tok_comma binarypredicate arithmetic_entity { $$ = new AST::AttributeList((AST::BinaryPredicate*)$2, (AST::Entity*)$3, nullptr); }
|   attributes tok_comma binarypredicate arithmetic_entity { $$ = new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, (AST::AttributeList*)$1); }
;

predicate: tok_identifier { $$ = new AST::Predicate(yytext); }
unarypredicate: tok_identifier { $$ = new AST::UnaryPredicate(yytext); }
binarypredicate: tok_identifier { $$ = new AST::BinaryPredicate(yytext); }

variable:
    tok_identifier { $$ = new AST::NamedVariable(yytext); }
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
|   tok_minus baseentity
;

mulentity:
    unaryentity
|   mulentity tok_times unaryentity
|   mulentity tok_div unaryentity
|   mulentity tok_mod unaryentity
;

plusentity:
    mulentity
|   plusentity tok_plus mulentity
|   plusentity tok_minus mulentity
;

sumentity:
    plusentity
|   tok_sum arithmetic_entity tok_in tok_open term tok_close
|   tok_count variable tok_in tok_open term tok_close
;

arithmetic_entity: sumentity;

value: 
    tok_string { 
        std::string value;
        for(int i=1; i<yyleng-1; ++i)
        {
            if(yytext[i]=='\\')
            {
                ++i;
                switch(yytext[i])
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
                    yyerror("Invalid escape character");
                    break;
                }
            }
            else
                value.push_back(yytext[i]);
        }
        $$ = new AST::String(value);
    }
|   tok_atstring { $$ = new AST::AtString(yytext+1); }
|   tok_integer { $$ = new AST::Integer(atoi(yytext)); }
|   tok_float   { $$ = new AST::Float(atof(yytext)); }
|   tok_true    { $$ = new AST::Bool(true); }
|   tok_false   { $$ = new AST::Bool(false); }
;

%%