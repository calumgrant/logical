
%{
#include "tokens.tab.h"
%}

%%

a return tok_a;
all return tok_all;
an return tok_an;
and return tok_and;
false           return tok_false;
find return tok_find;
has return tok_has;
if              return tok_if;
in return tok_in;
no return tok_no;
not return tok_not;
sum return tok_sum;
then return tok_then;
true            return tok_true;

\. return tok_dot;
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
[a-z]           return tok_identifier;
@[a-zA-Z_\-0-9]* return tok_identifier;
[0-9]+          return tok_integer;
\"([^"\\]|\\.)*\"   return tok_string;

\/\/[^\n]+\n

%%

// int yyFlexLexer::yywrap() { return 0; }

// void yyFlexLexer::error(int const & loc, const std::string &message) { }

