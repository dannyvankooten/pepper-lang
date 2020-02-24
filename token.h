#include <string.h>

enum token_type {
    TOKEN_ILLEGAL,
    TOKEN_EOF,
    TOKEN_IDENT,
    TOKEN_INT,
    TOKEN_FUNCTION,
    TOKEN_LET,
    TOKEN_TRUE,
    TOKEN_FALSE, 
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_EQ,
    TOKEN_NOT_EQ,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
};

static const char *token_names[] = {
    "ILLEGAL"
    "EOF"
    "IDENT"
    "INT"
    "FUNCTION"
    "LET"
    "TRUE"
    "FALSE"
    "IF"
    "ELSE"
    "RETURN"
    "="
    "+"
    "-"
    "!"
    "*"
    "/"
    "<"
    ">"
    "=="
    "!="
    ","
    ";"
    "("
    ")"
    "{"
    "}"
};

struct token {
    enum token_type type;
    char literal[32];
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
    } else {
        t->type = TOKEN_IDENT;
    }
}

const char *token_to_str(enum token_type type) {
    return token_names[type];
}