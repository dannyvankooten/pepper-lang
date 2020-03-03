#include <string.h> 
#include "token.h"

static const char *token_names[] = {
    "ILLEGAL",
    "EOF",
    "IDENT",
    "INT",
    "FUNCTION",
    "LET",
    "TRUE",
    "FALSE",
    "IF",
    "ELSE",
    "WHILE",
    "RETURN",
    "=",
    "+",
    "-",
    "!",
    "*",
    "/",
    "<",
    ">",
    "==",
    "!=",
    ",",
    ";",
    "(",
    ")",
    "{",
    "}",
    "STRING",
    "[",
    "]",
};

void get_ident(struct token *t) {
    // TODO: Proper implementation, using simple hashmap probably
    if (strcmp(t->literal, "let") == 0) {
       t->type = TOKEN_LET;
    } else if (strcmp(t->literal, "fn") == 0) {
        t->type = TOKEN_FUNCTION;
    } else if (strcmp(t->literal, "true") == 0) {
        t->type = TOKEN_TRUE;
    } else if (strcmp(t->literal, "false") == 0) {
        t->type = TOKEN_FALSE;
    } else if (strcmp(t->literal, "if") == 0) {
        t->type = TOKEN_IF;
    } else if (strcmp(t->literal, "else") == 0) {
        t->type = TOKEN_ELSE;
    } else if (strcmp(t->literal, "return") == 0) {
        t->type = TOKEN_RETURN;
    } else if (strcmp(t->literal, "while") == 0) {
        t->type = TOKEN_WHILE;
    } else {
        t->type = TOKEN_IDENT;
    }
}

const char *token_type_to_str(enum token_type type) {
    return token_names[type];
}