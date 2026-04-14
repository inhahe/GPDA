grammar JSON;

json : value EOF ;

value
    : obj
    | arr
    | STRING
    | NUMBER
    | 'true'
    | 'false'
    | 'null'
    ;

obj  : '{' pair (',' pair)* '}' | '{' '}' ;
pair : STRING ':' value ;
arr  : '[' value (',' value)* ']' | '[' ']' ;

STRING : '"' ( ESC | ~["\\] )* '"' ;
fragment ESC : '\\' . ;
NUMBER : '-'? ('0' | [1-9][0-9]*) ('.' [0-9]+)? ([eE][+\-]?[0-9]+)? ;
WS : [ \t\r\n]+ -> skip ;
