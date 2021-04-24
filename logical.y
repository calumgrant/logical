// %skeleton "lalr1.cc"

%locations
// %pure-parser

%{
#include "tokens.tab.h"

#include <iostream>

int yylex();
void yyerror(const char*) { std::cerr << "Syntax error at ?????"; }

// int yylex(YYLVAL * val, YYLTYPE * loc);
//void yyerror(YYLTYPE *loc, const char*) { std::cerr << "Syntax error at ?????"; }

//    #define YYSTYPE bool
//    void yyerror (yyscan_t yyscanner, char const *msg);
%}


%token tok_identifier tok_atentity tok_string tok_integer tok_float tok_underscore
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_sum tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false tok_count

%%

statementsopt: 
|   statements
;

statements:
    statement
|   statements statement
;

statement:
    fact //{ printf("Parsed a fact\n"); }
|   rule //{ printf("Parsed a rule\n"); }
|   datalog
|   query
;

datalog:
    datalog_predicate tok_dot
|   datalog_rule tok_dot
|   tok_questiondash datalog_predicate tok_dot
;

datalog_predicate:
    tok_identifier tok_open tok_close
|   tok_identifier tok_open termlist tok_close
;

termlist:
    arithmetic_term
|   termlist tok_comma arithmetic_term
;

datalog_rule:
    datalog_predicate tok_colondash datalog_clause
;

datalog_base_clause:
    datalog_predicate
|   term comparator term
|   tok_open datalog_clause tok_close
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

fact: baseclause tok_dot;

rule: tok_if clause tok_then baseclause tok_dot | 
baseclause tok_if clause tok_dot;

baseclause:
    term is_a unarypredicate
|   term is_a entity
|   unarypredicatelist term is_a unarypredicate
|   arithmetic_term comparator arithmetic_term
|   unarypredicatelist term
|   term has_a binarypredicate
|   term tok_comma binarypredicate
|   unarypredicatelist term has_a binarypredicate arithmetic_term
|   unarypredicatelist term has_a binarypredicate arithmetic_term attributes
|   unarypredicatelist term attributes
|   term has_a binarypredicate arithmetic_term
|   term has_a binarypredicate arithmetic_term attributes
|   term attributes
|   tok_open clause tok_close
;

unarypredicatelist:
    unarypredicate
|   unarypredicatelist unarypredicate
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
|   andclause tok_and notclause
;

orclause:
    andclause
|   orclause tok_or andclause
;

clause: orclause;

// Example: person x has name y, surname z

attributes:
    tok_comma binarypredicate arithmetic_term
|   attributes tok_comma binarypredicate arithmetic_term
;

predicate: tok_identifier
unarypredicate: tok_identifier;
binarypredicate: tok_identifier;
variable: tok_identifier | tok_underscore;

term: entity | variable;

baseterm:
    term
|   tok_open arithmetic_term tok_close
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

entity: tok_string | tok_atentity | tok_integer | tok_float | tok_true | tok_false;

%%