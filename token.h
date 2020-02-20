#include <string.h>
#undef EOF

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
    switch (type) {
        case TOKEN_ILLEGAL: return "ILLEGAL"; break;
        case TOKEN_EOF: return "EOF"; break;
        case TOKEN_IDENT: return "IDENT"; break;
        case TOKEN_INT: return "INT"; break;
        case TOKEN_FUNCTION: return "FUNCTION"; break;
        case TOKEN_LET: return "LET"; break;
        case TOKEN_TRUE: return "TRUE"; break;
        case TOKEN_FALSE: return "FALSE"; break;
        case TOKEN_IF: return "IF"; break;
        case TOKEN_ELSE: return "ELSE"; break;
        case TOKEN_RETURN: return "RETURN"; break;
        case TOKEN_ASSIGN: return "="; break;
        case TOKEN_PLUS: return "+"; break;
        case TOKEN_MINUS: return "-"; break;
        case TOKEN_BANG: return "!"; break;
        case TOKEN_ASTERISK: return "*"; break;
        case TOKEN_SLASH: return "/"; break;
        case TOKEN_LT: return "<"; break;
        case TOKEN_GT: return ">"; break;
        case TOKEN_EQ: return "=="; break;
        case TOKEN_NOT_EQ: return "!="; break;
        case TOKEN_COMMA: return ","; break;
        case TOKEN_SEMICOLON: return ";"; break;
        case TOKEN_LPAREN: return "("; break;
        case TOKEN_RPAREN: return ")"; break;
        case TOKEN_LBRACE: return "{"; break;
        case TOKEN_RBRACE: return "}"; break;
     }

     return "Invalid token";
}