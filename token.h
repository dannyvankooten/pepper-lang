#include <string.h>
#undef EOF

typedef enum {
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
}  token_type ;

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

const char * token_to_str(token_type type) {
    switch (type) {
        case ILLEGAL: return "ILLEGAL"; break;
        case EOF: return "EOF"; break;
        case IDENT: return "IDENT"; break;
        case INT: return "INT"; break;
        case FUNCTION: return "FUNCTION"; break;
        case LET: return "LET"; break;
        case TRUE: return "TRUE"; break;
        case FALSE: return "FALSE"; break;
        case IF: return "IF"; break;
        case ELSE: return "ELSE"; break;
        case RETURN: return "RETURN"; break;
        case ASSIGN: return "="; break;
        case PLUS: return "+"; break;
        case MINUS: return "-"; break;
        case BANG: return "!"; break;
        case ASTERISK: return "*"; break;
        case SLASH: return "/"; break;
        case LT: return "<"; break;
        case GT: return ">"; break;
        case EQ: return "=="; break;
        case NOT_EQ: return "!="; break;
        case COMMA: return ","; break;
        case SEMICOLON: return ";"; break;
        case LPAREN: return "("; break;
        case RPAREN: return ")"; break;
        case LBRACE: return "{"; break;
        case RBRACE: return "}"; break;
     }

     return "Invalid token";
}