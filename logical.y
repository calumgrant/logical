
%token identifier  atidentifier string integer float
%token if and has or not a an is dot then find
%token open close comma colondash semicolon equals notequals questiondash lt gt lteq gteq
%token times plus minus div mod
%%

statements:
    statement | statements statement ;

statement:
    fact
|   rule
|   datalog
|   query
;

datalog:
    datalog_predicate dot
|   datalog_rule dot
|   questiondash datalog_predicate dot
;

datalog_predicate:
    identifier open close
|   identifier open termlist close
;

termlist:
    arithmetic_term
|   termlist comma arithmetic_term
;

datalog_rule:
    datalog_predicate colondash datalog_clause dot
;

datalog_base_clause:
    datalog_predicate
|   term comparator term
|   open datalog_clause close
;

comparator:
    equals | notequals | lt | gt | lteq | gteq
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

query:
    find clause dot
|   find predicate dot
;

fact: baseclause dot;

rule: if clause then baseclause | baseclause if clause;

baseclause:
    term is unarypredicate
|   term is entity
|   unarypredicate term
|   term has binarypredicate arithmetic_term
|   term has binarypredicate arithmetic_term attributes
;

notclause:
    baseclause
|   not baseclause
;

andclause:
    notclause
|   andclause and notclause
;

orclause:
    andclause
|   orclause or andclause
;

clause: orclause;

// person x has name y, surname z

attributes:
    comma binarypredicate arithmetic_term
|   attributes comma binarypredicate arithmetic_term
;

predicate: identifier
unarypredicate: identifier;
binarypredicate: identifier;
variable: identifier;

term: entity | variable;

baseterm:
    term
|   open arithmetic_term close
;

unaryterm:
    baseterm
|    minus baseterm
;

multerm:
    unaryterm
|   multerm times unaryterm
|   multerm div unaryterm
|   multerm mod unaryterm
;

plusterm:
    multerm
|   plusterm plus multerm
|   plusterm minus multerm
;

arithmetic_term: plusterm;


entity: string | atidentifier | integer | float;

%%