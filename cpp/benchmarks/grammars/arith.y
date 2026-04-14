%{
#include <stdio.h>
#include <stdlib.h>
int arith_lex(void);
void arith_error(const char* s) { fprintf(stderr, "bison error: %s\n", s); }
%}

%define api.prefix {arith_}

%token NUMBER PLUS MINUS STAR SLASH LPAREN RPAREN

%left PLUS MINUS
%left STAR SLASH

%%

main
    : expr
    ;

expr
    : expr PLUS expr
    | expr MINUS expr
    | expr STAR expr
    | expr SLASH expr
    | NUMBER
    | LPAREN expr RPAREN
    ;

%%
