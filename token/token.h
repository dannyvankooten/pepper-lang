#include <string.h>

typedef struct Token {
    char type[100];
    char literal[100];
} token;

const char * ILLEGAL = "ILLEGAL";
const char * EOF = "EOF";
const char * IDENT = "IDENT";
const char * INT = "INT";
const char * ASSIGN = "=";
const char * PLUS = "+";
const char * COMMA = ",";
const char * SEMICOLON = ";";
const char * LPAREN = "(";
const char * RPAREN = ")";
const char * LBRACE = "{";
const char * RBRACE = "}";
const char * FUNCTION = "FUNCTION";
const char * LET = "LET";

void get_ident(token *t) {
    // TODO: Proper implementation, using simple hashmap probably
    if (strcmp(t->literal, "let") == 0) {
        strcpy(t->type, LET);
    } else if (strcmp(t->literal, "fn") == 0) {
        strcpy(t->type, FUNCTION);
    } else {
        strcpy(t->type, IDENT);
    }
}
