#include <string.h>
#undef EOF
typedef struct Token {
    char type[100];
    char literal[100];
} token;

const char * ILLEGAL = "ILLEGAL";
const char * EOF = "EOF";
const char * IDENT = "IDENT";
const char * INT = "INT";

// keywords
const char * FUNCTION = "FUNCTION";
const char * LET = "LET";
const char * TRUE = "TRUE";
const char * FALSE = "FALSE";
const char * IF = "IF";
const char * ELSE = "ELSE";
const char * RETURN = "RETURN";

// operators
const char * ASSIGN = "=";
const char * PLUS = "+";
const char * MINUS = "-";
const char * BANG = "!";
const char * ASTERISK = "*";
const char * SLASH = "/";

const char * LT = "<";
const char * GT = ">";
const char * LTE = "<=";
const char * GTE = ">=";
const char * EQ = "==";
const char * NOT_EQ = "!=";

const char * COMMA = ",";
const char * SEMICOLON = ";";
const char * LPAREN = "(";
const char * RPAREN = ")";
const char * LBRACE = "{";
const char * RBRACE = "}";

void get_ident(token *t) {
    // TODO: Proper implementation, using simple hashmap probably
    if (strcmp(t->literal, "let") == 0) {
        strcpy(t->type, LET);
    } else if (strcmp(t->literal, "fn") == 0) {
        strcpy(t->type, FUNCTION);
    } else if (strcmp(t->literal, "true") == 0) {
        strcpy(t->type, TRUE);
    } else if (strcmp(t->literal, "false") == 0) {
        strcpy(t->type, FALSE);
    } else if (strcmp(t->literal, "if") == 0) {
        strcpy(t->type, IF);
    } else if (strcmp(t->literal, "else") == 0) {
        strcpy(t->type, ELSE);
    } else if (strcmp(t->literal, "return") == 0) {
        strcpy(t->type, RETURN);
    } else {
        strcpy(t->type, IDENT);
    }
}
