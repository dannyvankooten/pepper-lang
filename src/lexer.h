#pragma once

#include <stdint.h>

#define MAX_IDENT_LENGTH 64

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
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_GT,
    TOKEN_GTE,
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
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
};

struct token {
    enum token_type type;
    char literal[MAX_IDENT_LENGTH];
    const char *start;
    const char *end;
};

struct lexer {
    const char *input;
    uint32_t pos;
    uint32_t cur_lineno;    
};

int gettoken(struct lexer *l, struct token *t);
struct lexer new_lexer(const char *input);
const char *token_type_to_str(enum token_type type);