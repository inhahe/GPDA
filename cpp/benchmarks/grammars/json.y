%{
#include <stdio.h>
#include <stdlib.h>
int yylex(void);
void yyerror(const char* s) { fprintf(stderr, "bison error: %s\n", s); }
%}

%token LBRACE RBRACE LBRACKET RBRACKET COMMA COLON
%token TRUE_KW FALSE_KW NULL_KW STRING NUMBER

%%

json
    : value
    ;

value
    : object
    | array
    | STRING
    | NUMBER
    | TRUE_KW
    | FALSE_KW
    | NULL_KW
    ;

object
    : LBRACE RBRACE
    | LBRACE pair_list RBRACE
    ;

pair_list
    : pair
    | pair_list COMMA pair
    ;

pair
    : STRING COLON value
    ;

array
    : LBRACKET RBRACKET
    | LBRACKET value_list RBRACKET
    ;

value_list
    : value
    | value_list COMMA value
    ;

%%
