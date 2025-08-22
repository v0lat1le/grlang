# GrLang
Sea of nodes optimizing compiler shenanings

[![CI](https://github.com/v0lat1le/grlang/actions/workflows/ci.yaml/badge.svg)](https://github.com/v0lat1le/grlang/actions/workflows/ci.yaml)

```antlrv4
grammar GrLang;
program: statement+ EOF;
statement:
    '{' statement+ '}'
    | 'return' expression
    | 'if' expression statement ('else' statement)?
    | 'while' expression statement
    | 'break'
    | 'continue'
    | IDENTIFIER ':' type '=' expression
    | IDENTIFIER ':=' expression
    | IDENTIFIER '=' expression
;
expression:
    '(' expression ')'
    | ('-' | '!') expression
    | expression ('+' | '==') expression
    | IDENTIFIER
    | INTEGER_LITERAL
;
type : 'int';
IDENTIFIER : NON_DIGIT (NON_DIGIT | DIGIT)*;
INTEGER_LITERAL : DIGIT+;
NON_DIGIT: [a-zA-Z_];
DIGIT: [0-9];
```
