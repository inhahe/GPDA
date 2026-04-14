%{
#include "json.tab.h"
#include <stdlib.h>
#include <string.h>
%}

%option noyywrap nounput noinput

%%
[ \t\n\r]+          { /* skip whitespace */ }
"{"                 { return LBRACE; }
"}"                 { return RBRACE; }
"["                 { return LBRACKET; }
"]"                 { return RBRACKET; }
","                 { return COMMA; }
":"                 { return COLON; }
"true"              { return TRUE_KW; }
"false"             { return FALSE_KW; }
"null"              { return NULL_KW; }
\"(\\.|[^"\\])*\"   { return STRING; }
-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+\-]?[0-9]+)?   { return NUMBER; }
.                   { return yytext[0]; /* error */ }
%%
