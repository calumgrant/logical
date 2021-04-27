
%{
#define register

#include "tokens.tab.h"
// # %option reentrant bison-bridge
%}

%option nodefault
%option yylineno

%%

a return tok_a;
all return tok_all;
an return tok_an;
and return tok_and;
count return tok_count;
false           return tok_false;
find return tok_find;
has return tok_has;
if     return tok_if;
in return tok_in;
is  return tok_is;
no return tok_no;
not return tok_not;
or return tok_or;
sum return tok_sum;
then return tok_then;
true            return tok_true;
with return tok_with;

\. { return tok_dot; }
\( return tok_open;
\) return tok_close;
, return tok_comma;
:- return tok_colondash;
\?- return tok_questiondash;
; return tok_semicolon;

= return tok_equals;
!= return tok_notequals;
\< return tok_lt;
> return tok_gt;
\<= return tok_lteq;
>= return tok_gteq;

\+ return tok_plus;
- return tok_minus;
\* return tok_times;
\/ return tok_div;
% return tok_mod;

_               return tok_underscore;
[a-zA-Z][a-zA-Z0-9_\-]*|_[a-zA-Z0-9_]+ { yylval.sval = strdup(yytext); return tok_identifier; }

@[a-zA-Z_\-0-9]* { yylval.sval = strdup(yytext); return tok_atstring; }
-?[0-9]+          { yylval.ival = atoi(yytext); return tok_integer; }
\"([^"\\]|\\.)*\"  { yylval.sval = strdup(yytext); return tok_string; }

[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+ { yylval.fval = atof(yytext); return tok_float; }

\/\/[^\n]+
[ \r\n\t]+
.

%%

