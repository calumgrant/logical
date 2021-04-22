
%token identifier  atidentifier string integer float
%token if and has or not a an is dot then
%token open close comma colondash semicolon equals

%%

statements:
    statement | statements statement ;

statement:
    fact
|   rule
|   datalog
;

datalog:
    datalog_predicate dot
|   datalog_rule dot
;

datalog_predicate:
    identifier open close
|   identifier open termlist close
;

termlist:
    term
|   termlist comma term
;

datalog_rule:
    datalog_predicate colondash datalog_clause dot
;

datalog_base_clause:
    datalog_predicate
|   term equals term
|   open datalog_clause close
;

datalog_unary_clause:
    datalog_base_clause
|   not datalog_base_clause
;

datalog_and_clause:
    datalog_unary_clause
|   datalog_and_clause and datalog_unary_clause
|   datalog_and_clause comma datalog_unary_clause
;

datalog_clause:
|   datalog_clause or datalog_and_clause
|   datalog_clause semicolon datalog_and_clause
|   datalog_and_clause
;

fact: clause dot;

rule: if clause then clause | clause if clause;

// ?? factclause
clause:
    term is unarypredicate
|   unarypredicate term
|   term has binarypredicate entity
|   term has binarypredicate entity andfacts
// |   clause and clause
// |   clause or clause
;

// person x has name y and surname z

andfacts:
    and binarypredicate entity
|   andfacts and binarypredicate entity
;

unarypredicate: identifier;
binarypredicate: identifier;
variable: identifier;

term: entity | variable;

entity: string | atidentifier | integer | float;

%%