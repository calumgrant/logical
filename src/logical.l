
%{
#define register

#include "tokens.tab.h"
#include "Database.hpp"

#define YY_EXTRA_TYPE ParseData*

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = yycolumn; yylloc->last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option nodefault
%option yylineno
%option reentrant bison-bridge bison-locations

IDENTIFIER   [a-zA-Z_]([a-zA-Z0-9_]|[a-zA-Z]-[a-zA-Z])*
%%

a       return tok_a;
all     return tok_all;
an      return tok_an;
and     return tok_and;
count   return tok_count;
false   return tok_false;
find    return tok_find;
has     return tok_has;
if      return tok_if;
in      return tok_in;
is      return tok_is;
new     return tok_new;
no      return tok_no;
not     return tok_not;
or      return tok_or;
reaches return tok_reaches;
sum     return tok_sum;
then    return tok_then;
true    return tok_true;

\.      return tok_dot;
\(      return tok_open;
\)      return tok_close;
,       return tok_comma;
:-      return tok_colondash;
\?-     return tok_questiondash;
;       return tok_semicolon;

=       return tok_equals;
!=      return tok_notequals;
\<      return tok_lt;
>       return tok_gt;
\<=     return tok_lteq;
>=      return tok_gteq;

\+      return tok_plus;
--?     return tok_minus;
\*      return tok_times;
\/      return tok_div;
%       return tok_mod;
\[       return tok_open_square;
]       return tok_close_square;

_       return tok_underscore;
{IDENTIFIER}|{IDENTIFIER}?(:{IDENTIFIER})+ { yylval->ival = yyextra->db.GetStringId(yytext); return tok_identifier; }

@[a-zA-Z_\-0-9]*    { yylval->ival = yyextra->db.GetAtStringId(yytext+1); return tok_atstring; }
[0-9]+            { yylval->ival = atoi(yytext); return tok_integer; }
\"([^"\\]|\\.)*\"   { yylval->ival = yyextra->db.GetStringLiteral(yytext); return tok_string; }

[0-9]*\.[0-9]+([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+  { yylval->fval = atof(yytext); return tok_float; }

\/\/[^\n]*
\n      { yycolumn = 1; }
[ \r\t]+
.

%%
