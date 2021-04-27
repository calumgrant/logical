// %skeleton "lalr1.cc"

%locations
// %pure-parser


%{
#include <Clause.hpp>
#define YYSTYPE AST::Node*

// This is totally wrong I think
extern char yytext[];

#include "tokens.tab.h"
#include <memory>

#include <iostream>


// std::unique_ptr<AST::Clause>

int yylex();
extern int yylineno;
extern int yyleng;
void yyerror(const char*) { std::cerr << "Syntax error at line " << yylineno; }

// int yylex(YYLVAL * val, YYLTYPE * loc);
//void yyerror(YYLTYPE *loc, const char*) { std::cerr << "Syntax error at ?????"; }

//    #define YYSTYPE bool
//    void yyerror (yyscan_t yyscanner, char const *msg);

// What to do
void ProcessFact(AST::Clause * statement);
void ProcessRule(AST::Clause * lhs, AST::Clause * rhs);

%}


%token tok_identifier tok_atentity tok_string tok_integer tok_float tok_underscore
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
    datalog_predicate tok_dot { ProcessFact((AST::Clause*)$1); }
|   datalog_rule tok_dot
|   tok_questiondash datalog_predicate tok_dot
;

datalog_predicate:
    predicate tok_open tok_close { $$ = new AST::DatalogPredicate((AST::Predicate*)$1, nullptr); }
|   predicate tok_open termlist tok_close { $$ = new AST::DatalogPredicate((AST::Predicate*)$1, (AST::EntityList*)$3); }
;

termlist:
    arithmetic_term { $$ = new AST::EntityList((AST::Entity*)$1); }
|   termlist tok_comma arithmetic_term { ((AST::EntityList*)$1)->Add((AST::Entity*)$3); }
;

datalog_rule:
    datalog_predicate tok_colondash datalog_clause
    {
        ProcessRule((AST::Clause*)$1,(AST::Clause*)$3);
    }
;

datalog_base_clause:
    datalog_predicate
|   term comparator term
|   tok_open datalog_clause tok_close { $$ = $2; }
;

comparator:
    tok_equals | tok_notequals | tok_lt | tok_gt | tok_lteq | tok_gteq
    ;

datalog_unary_clause:
    datalog_base_clause
|   tok_not datalog_base_clause
;

datalog_and_clause:
    datalog_unary_clause
|   datalog_and_clause tok_and datalog_unary_clause
|   datalog_and_clause tok_comma datalog_unary_clause
;

datalog_clause:
|   datalog_clause tok_or datalog_and_clause
|   datalog_clause tok_semicolon datalog_and_clause
|   datalog_and_clause
;

query:
    tok_find queryclause tok_dot
|   tok_find predicate tok_dot
|   tok_find queryclause tok_if clause tok_dot
|   tok_find variablelist tok_if clause tok_dot  // find A, surname S could be am attribute or a variable list 
;

// Different syntax to distinguish them from variable lists A, B, C
querybaseclause:
    unarypredicate variable
|   unarypredicate variable has_a binarypredicate variable
|   unarypredicate variable has_a binarypredicate variable attributes
|   unarypredicate variable attributes
|   variable has_a binarypredicate variable
|   variable has_a binarypredicate variable attributes
;

queryclause:
    querybaseclause
|   queryclause tok_and querybaseclause
;

variablelist:
    variable
|   variablelist tok_comma variable
;

fact: clause tok_dot { ProcessFact((AST::Clause*)$1); };

rule:
    tok_if clause tok_then clause tok_dot { ProcessRule((AST::Clause*)$4, (AST::Clause*)$2); }
|   clause tok_if clause tok_dot { ProcessRule((AST::Clause*)$1, (AST::Clause*)$3); }
;

baseclause:
    term is_a unarypredicate { $$ = new AST::TermIs((AST::Entity*)$1, (AST::UnaryPredicate*)$3); }
|   term is_a entity { $$ = new AST::NotImplementedClause($1, $3); }
|   unarypredicatelist term is_a unarypredicate { $$ = new AST::TermIsPredicate((AST::Entity*)$2, (AST::UnaryPredicateList*)$1, (AST::UnaryPredicate*)$4); }
|   arithmetic_term comparator arithmetic_term { $$ = new AST::NotImplementedClause($1, $3); }
|   unarypredicatelist term { $$ = new AST::TermIs((AST::Entity*)$2, (AST::UnaryPredicateList*)$1); }
|   term has_a binarypredicate { $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, new AST::AttributeList((AST::BinaryPredicate*)$3, nullptr, nullptr)); }
|   term tok_comma binarypredicate
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, new AST::AttributeList((AST::BinaryPredicate*)$3, nullptr, nullptr));
    }
|   unarypredicatelist term has_a binarypredicate arithmetic_term
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, 
            new AST::AttributeList((AST::BinaryPredicate*)$4, (AST::Entity*)$5, nullptr));
    }
|   unarypredicatelist term has_a binarypredicate arithmetic_term tok_with inlist
    {
        $$ = new AST::NotImplementedClause();
    }
|   unarypredicatelist term has_a binarypredicate arithmetic_term attributes
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, 
            new AST::AttributeList((AST::BinaryPredicate*)$4, (AST::Entity*)$5, (AST::AttributeList*)$6));
    }
|   unarypredicatelist term attributes 
    { 
        $$ = new AST::EntityHasAttributes((AST::UnaryPredicateList*)$1, (AST::Entity*)$2, (AST::AttributeList*)$3);
    }
|   term has_a binarypredicate arithmetic_term
    { 
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, 
            new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, nullptr));
    }
|   term has_a binarypredicate arithmetic_term tok_with inlist
    { 
        $$ = new AST::NotImplementedClause();
    }
|   term has_a binarypredicate arithmetic_term attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, 
            new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, (AST::AttributeList*)$5));
    }
|   term attributes
    {
        $$ = new AST::EntityHasAttributes(nullptr, (AST::Entity*)$1, (AST::AttributeList*)$2);
    }
|   tok_open clause tok_close { $$=$2; }
;

unarypredicatelist:
    unarypredicate { $$ = new AST::UnaryPredicateList((AST::UnaryPredicate*)$1); }
|   unarypredicatelist unarypredicate { $$=$1; ((AST::UnaryPredicateList*)$$)->Append((AST::UnaryPredicate*)$2); }
;

inlist:
    unarypredicate term
|   inlist tok_comma unarypredicate term
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
;

notclause:
    allclause
|   tok_not allclause
;

andclause:
    notclause
|   andclause tok_and notclause { $$ = new AST::And((AST::Clause*)$1, (AST::Clause*)$3); }
;

orclause:
    andclause
|   orclause tok_or andclause
;

clause: orclause;

// Example: person x has name y, surname z

attributes:
    tok_comma binarypredicate arithmetic_term { $$ = new AST::AttributeList((AST::BinaryPredicate*)$2, (AST::Entity*)$3, nullptr); }
|   attributes tok_comma binarypredicate arithmetic_term { $$ = new AST::AttributeList((AST::BinaryPredicate*)$3, (AST::Entity*)$4, (AST::AttributeList*)$1); }
;

predicate: tok_identifier { $$ = new AST::Predicate(yytext); }
unarypredicate: tok_identifier { $$ = new AST::UnaryPredicate(yytext); }
binarypredicate: tok_identifier { $$ = new AST::BinaryPredicate(yytext); }

variable:
    tok_identifier { $$ = new AST::NamedVariable(yytext); }
|   tok_underscore { $$ = new AST::UnnamedVariable(); }
;

term:
    entity
|   variable

baseterm:
    term
|   tok_open arithmetic_term tok_close { $$ = $2; }
;

unaryterm:
    baseterm
|   tok_minus baseterm
;

multerm:
    unaryterm
|   multerm tok_times unaryterm
|   multerm tok_div unaryterm
|   multerm tok_mod unaryterm
;

plusterm:
    multerm
|   plusterm tok_plus multerm
|   plusterm tok_minus multerm
;

sumterm:
    plusterm
|   tok_sum arithmetic_term tok_in tok_open clause tok_close
|   tok_count variable tok_in tok_open clause tok_close
;

arithmetic_term: sumterm;

entity: 
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
|   tok_atentity { $$ = new AST::AtEntity(yytext+1); }
|   tok_integer { $$ = new AST::Integer(atoi(yytext)); }
|   tok_float   { $$ = new AST::Float(atof(yytext)); }
|   tok_true    { $$ = new AST::Bool(true); }
|   tok_false   { $$ = new AST::Bool(false); }
;

%%