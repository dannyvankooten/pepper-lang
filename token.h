#include <string.h>
#undef EOF

enum TokenType {
    ILLEGAL,
    EOF,
    IDENT,
    INT,
    FUNCTION,
    LET,
    TRUE,
    FALSE, 
    IF,
    ELSE,
    RETURN,
    ASSIGN,
    PLUS,
    MINUS,
    BANG,
    ASTERISK,
    SLASH,
    LT,
    GT,
    EQ,
    NOT_EQ,
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE
};

typedef struct Token {
    int type;
    char literal[100];
} token;

void get_ident(token *t) {
    // TODO: Proper implementation, using simple hashmap probably
    if (strcmp(t->literal, "let") == 0) {
       t->type = LET;
    } else if (strcmp(t->literal, "fn") == 0) {
        t->type = FUNCTION;
    } else if (strcmp(t->literal, "true") == 0) {
        t->type = TRUE;
    } else if (strcmp(t->literal, "false") == 0) {
        t->type = FALSE;
    } else if (strcmp(t->literal, "if") == 0) {
        t->type = IF;
    } else if (strcmp(t->literal, "else") == 0) {
        t->type = ELSE;
    } else if (strcmp(t->literal, "return") == 0) {
        t->type = RETURN;
    } else {
        t->type = IDENT;
    }
}
