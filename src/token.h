#ifndef TOKEN_H
#define TOKEN_H

#define MAX_LITERAL_LENGTH 255 

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
    TOKEN_STRING,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
};


struct token {
    enum token_type type;
    char literal[MAX_LITERAL_LENGTH];
};

void get_ident(struct token *t);

const char *token_type_to_str(enum token_type type);

#endif