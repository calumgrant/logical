// %skeleton "lalr1.cc"

%{
// #include <FlexLexer.h>

int yylex(...) { return 0;}

void yyerror(const char*) { }
%}


%token tok_identifier tok_atentity tok_string tok_integer tok_float tok_underscore
%token tok_if tok_and tok_has tok_or tok_not tok_a tok_an tok_no tok_is tok_dot tok_then tok_find tok_sum tok_in tok_all
%token tok_open tok_close tok_comma tok_colondash tok_semicolon tok_equals tok_notequals tok_questiondash tok_lt tok_gt tok_lteq tok_gteq
%token tok_times tok_plus tok_minus tok_div tok_mod tok_true tok_false

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
    datalog_predicate tok_colondash datalog_clause tok_dot
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
    tok_find clause tok_dot
|   tok_find predicate tok_dot
|   tok_find baseclause tok_if clause tok_dot
|   tok_find variable tok_if clause tok_dot
;

fact: baseclause tok_dot;

rule: tok_if clause tok_then baseclause | baseclause tok_if clause;

baseclause:
    term is_a unarypredicate
|   term is_a entity
|   unarypredicate term
// |   term has_a binarypredicate
|   term has_a binarypredicate arithmetic_term
|   term has_a binarypredicate arithmetic_term attributes
|   term attributes
|   tok_open clause tok_close
|   tok_all baseclause tok_in baseclause 
;

has_a:
    tok_has
|   tok_has tok_a
|   tok_has tok_an
;

is_a:
    tok_is
|   tok_is tok_a
|   tok_is tok_an
|   tok_in
;

notclause:
    baseclause
|   tok_not baseclause
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
;

arithmetic_term: sumterm;

entity: tok_string | tok_atentity | tok_integer | tok_float | tok_true | tok_false;

%%