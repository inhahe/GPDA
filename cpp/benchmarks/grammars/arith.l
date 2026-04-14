%{
#include "arith.tab.h"
#include <stdlib.h>
#include <string.h>
%}

%option prefix="arith_"
%option noyywrap nounput noinput

%%
[ \t\n\r]+         { /* skip whitespace */ }
[0-9]+             { return NUMBER; }
"+"                { return PLUS; }
"-"                { return MINUS; }
"*"                { return STAR; }
"/"                { return SLASH; }
"("                { return LPAREN; }
")"                { return RPAREN; }
.                  { return yytext[0]; /* error */ }
%%
