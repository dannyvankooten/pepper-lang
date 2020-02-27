#ifndef PARSER_H 
#define PARSER_H

#include <stdbool.h>
#include "lexer.h"

#define MAX_IDENT_LENGTH 32

enum precedence {
    LOWEST = 1,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL,           // fn()
    INDEX,          // array[i]
};

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
    EXPR_WHILE,
};

enum statement_type {
    STMT_LET = 1,
    STMT_RETURN,
    STMT_EXPR,
};

enum operator {
    OP_UNKNOWN,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_GT,
    OP_LT,
    OP_EQ,
    OP_NOT_EQ,
    OP_NEGATE,
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
    unsigned int size;
    unsigned int cap;
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
    unsigned int size;
    unsigned int cap;
};

struct if_expression {
    struct expression *condition;
    struct block_statement *consequence;
    struct block_statement *alternative;
};

struct function_literal {
    struct identifier_list parameters;
    struct block_statement *body;
};

struct expression_list {
    unsigned int size;
    unsigned int cap;
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

struct expression {
    enum expression_type type;
    struct token token;
    union {
        long integer;
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
        struct while_expression whilst;
    };
} expression;

struct program {
    struct statement *statements;
    unsigned int cap;
    unsigned int size;
};

struct parser {
    struct lexer *lexer;
    struct token current_token;
    struct token next_token;

    // TODO: allocate this dynamically
    unsigned int errors;
    char error_messages[8][128];
};

struct parser new_parser(struct lexer *l);
struct program *parse_program(struct parser *parser);
void block_statement_to_str(char *str, struct block_statement *b);
void identifier_list_to_str(char *str, struct identifier_list *identifiers);
char *program_to_str(struct program *p);
void free_program(struct program *p);
char *operator_to_str(enum operator operator);

#endif