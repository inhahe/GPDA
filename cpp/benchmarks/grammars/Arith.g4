grammar Arith;

main : expr EOF ;

// ANTLR4 supports direct left recursion, and resolves operator precedence
// via alternative ordering (top-most alternative = lowest precedence).
// These are ordered * / above + - to give */ higher precedence.
expr
    : expr ('*' | '/') expr
    | expr ('+' | '-') expr
    | NUMBER
    | '(' expr ')'
    ;

NUMBER : [0-9]+ ;
WS     : [ \t\r\n]+ -> skip ;
