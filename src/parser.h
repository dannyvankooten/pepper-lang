#pragma once 

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "lexer.h"

enum expression_type {
    EXPR_INFIX = 1,
    EXPR_PREFIX,
    EXPR_INT,
    EXPR_IDENT,
    EXPR_BOOL,
    EXPR_IF,
    EXPR_FUNCTION,
    EXPR_CALL,
    EXPR_STRING,
    EXPR_ARRAY,
    EXPR_INDEX,
    EXPR_FOR,
    EXPR_WHILE,
    EXPR_ASSIGN,
};

enum statement_type {
    STMT_LET = 1,
    STMT_RETURN,
    STMT_EXPR,
    STMT_BREAK,
    STMT_CONTINUE,
};

enum operator {
    OP_UNKNOWN,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_GT,
    OP_GTE,
    OP_LT,
    OP_LTE,
    OP_EQ,
    OP_NOT_EQ,
    OP_NEGATE,
    OP_AND,
    OP_OR,
    OP_MODULO,
};

struct prefix_expression {
    enum operator operator;
    struct expression *right;
};

struct infix_expression {
    enum operator operator;
    struct expression *left;
    struct expression *right;
};

struct identifier {
    char value[MAX_IDENT_LENGTH];
    struct token token;
};

struct identifier_list {
    struct identifier *values;
    uint32_t size;
    uint32_t cap;
};

struct statement {
    enum statement_type type;
    struct token token;
    struct identifier name;
    struct expression *value;
};

struct block_statement {
    struct token token;
    struct statement *statements;
    uint32_t size;
    uint32_t cap;
};

struct if_expression {
    struct expression *condition;
    struct block_statement *consequence;
    struct block_statement *alternative;
};

struct function_literal {
    char name[MAX_IDENT_LENGTH];
    struct identifier_list parameters;
    struct block_statement *body;
};

struct expression_list {
    uint32_t size;
    uint32_t cap;
    struct expression **values;
};

struct call_expression {
    struct expression *function;
    struct expression_list arguments;
};

struct index_expression {
    struct expression *left;
    struct expression *index;
};

struct while_expression {
    struct expression *condition;
    struct block_statement *body;
};

struct for_expression {
    struct statement init;
    struct expression* condition;
    struct statement inc;
    struct block_statement *body;
};

struct assignment_expression {
    struct expression *left;
    struct expression *value;
};

struct expression {
    enum expression_type type;
    struct token token;
    union {
        int64_t integer;
        bool boolean;
        char *string;
        struct identifier ident;
        struct prefix_expression prefix;
        struct infix_expression infix;
        struct if_expression ifelse;
        struct function_literal function;
        struct call_expression call;
        struct expression_list array;
        struct index_expression index;
        struct while_expression while_loop;
        struct assignment_expression assign;
        struct for_expression for_loop;
    };
};

struct program {
    struct statement *statements;
    uint32_t cap;
    uint32_t size;
};

struct parser {
    struct lexer *lexer;
    struct token current_token;
    struct token next_token;

    uint32_t errors;
    char error_messages[4][64];
};

struct parser new_parser(struct lexer *l);
struct program *parse_program(struct parser *parser);
struct program *parse_program_str(const char *str);
void block_statement_to_str(char *str, const struct block_statement *b);
void identifier_list_to_str(char *str, const struct identifier_list *identifiers);
char *program_to_str(const struct program *p);
void free_program(struct program *p);
char *operator_to_str(const enum operator operator);