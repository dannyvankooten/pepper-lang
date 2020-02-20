#include "lexer.h"
#include <stdlib.h>

#define MAX_IDENT_LENGTH 32
#define MAX_OPERATOR_LENGTH 3

enum precedence {
    LOWEST = 1,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL            // fn()
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
};

enum statement_type {
    STMT_LET = 1,
    STMT_RETURN,
    STMT_EXPR,
};

struct bool_expression {
    struct token token;
    char value;
};

struct integer_literal {
    struct token token;
    int value;
};

struct prefix_expression {
    struct token token;
    char operator[MAX_OPERATOR_LENGTH];
    struct expression *right;
};

struct infix_expression {
    struct token token;
    char operator[MAX_OPERATOR_LENGTH];
    struct expression *left;
    struct expression *right;
};

struct identifier {
    struct token token; 
    char value[MAX_IDENT_LENGTH];
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
    struct token token;
    struct expression *condition;
    struct block_statement *consequence;
    struct block_statement *alternative;
};

struct function_literal {
    struct token token;
    struct identifier_list parameters;
    struct block_statement *body;
};

struct expression_list {
    unsigned int size;
    unsigned int cap;
    struct expression *values;
};

struct call_expresion {
    struct token token;
    struct expression *function;
    struct expression_list arguments;
};

struct expression {
    enum expression_type type;
    union {
        struct bool_expression bool;
        struct identifier ident;
        struct integer_literal integer;
        struct prefix_expression prefix;
        struct infix_expression infix;
        struct if_expression ifelse;
        struct function_literal function;
        struct call_expresion call;
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

typedef char operator[MAX_OPERATOR_LENGTH] ;

static enum precedence get_token_precedence(struct token t);
static void next_token(struct parser * p);
static int current_token_is(struct parser *p, enum token_type t);
static int next_token_is(struct parser *p, enum token_type t) ;
static int expect_next_token(struct parser *p, enum token_type t);
static void expression_to_str(char *str, struct expression *expr);
static struct expression * parse_expression(struct parser *p, int precedence);
static int parse_statement(struct parser *p, struct statement *s);
static void free_expression(struct expression *expr);

struct parser new_parser(struct lexer *l) {
    struct parser p = {
        .lexer = l,
    };
   
    // read two tokens so that both current_token and next_token are set
    next_token(&p);
    next_token(&p);
    return p;
}

static void next_token(struct parser * p) {
    p->current_token = p->next_token;
    gettoken(p->lexer, &p->next_token);
}

static int current_token_is(struct parser *p, enum token_type t) {
    return t == p->current_token.type;
}

static int next_token_is(struct parser *p, enum token_type t) {
    return t == p->next_token.type;
}

static int expect_next_token(struct parser *p, enum token_type t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return 1;
    }

    sprintf(p->error_messages[p->errors++], "expected next token to be %s, got %s instead", token_to_str(t), token_to_str(p->next_token.type));
    return 0;
}

static int parse_let_statement(struct parser *p, struct statement *s) {
    s->type = STMT_LET;
    s->token = p->current_token;

    if (!expect_next_token(p, IDENT)) {
        return -1;
    }

    // parse identifier
    struct identifier id = {
        .token = p->current_token,
    };
    strcpy(id.value, p->current_token.literal);
    s->name = id;

    if (!expect_next_token(p, ASSIGN)) {
        return -1;
    }

    // parse expression
    next_token(p);
    s->value = parse_expression(p, LOWEST);
    if (next_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static int parse_return_statement(struct parser *p, struct statement *s) {
    s->type = STMT_RETURN;
    s->token = p->current_token;

    // parse expression
    next_token(p);
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static struct expression *parse_identifier_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_IDENT;
    expr->ident.token = p->current_token;
    strncpy(expr->ident.value, p->current_token.literal, MAX_IDENT_LENGTH);
    return expr;
}

static struct expression *parse_int_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_INT;
    expr->integer.token = p->current_token;
    expr->integer.value = atoi(p->current_token.literal);
    return expr;
}

static struct expression *parse_prefix_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_PREFIX;
    expr->prefix.token = p->current_token;
    strncpy(expr->prefix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    next_token(p);
    expr->prefix.right = parse_expression(p, PREFIX);
    return expr;
}

struct expression_list parse_call_arguments(struct parser *p) {
    struct expression_list list = {
        .size = 0,
        .cap = 0,
    };

    if (next_token_is(p, RPAREN)) {
        next_token(p);
        return list;
    }

    // allocate memory here, so we do not need an alloc for calls without any arguments
    list.cap = 4;
    list.values = malloc(list.cap * sizeof(struct expression));
    
    next_token(p);
    list.values[list.size++] = *parse_expression(p, LOWEST);

    while (next_token_is(p, COMMA)) {
        next_token(p);
        next_token(p);

        list.values[list.size++] = *parse_expression(p, LOWEST);

        // double capacity if needed
        if (list.size >= list.cap) {
            list.cap *= 2;
            list.values = realloc(list.values, list.cap * sizeof(struct expression));
        }
    }

    if (!expect_next_token(p, RPAREN)) {
        free(list.values);
        return list;
    }

    return list;
}

struct expression *parse_call_expression(struct parser *p, struct expression *left) {
    struct expression *expr = malloc(sizeof (struct expression));
    expr->type = EXPR_CALL;
    expr->call.token = p->current_token;
    expr->call.function = left;
    expr->call.arguments = parse_call_arguments(p);
    return expr;
}

static struct expression *parse_infix_expression(struct parser *p, struct expression *left) {
    struct expression * expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_INFIX;
    expr->infix.left = left;
    expr->infix.token = p->current_token;
    strncpy(expr->infix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->infix.right = parse_expression(p, precedence);
    return expr;
}

struct expression *parse_boolean_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_BOOL;
    expr->bool.token = p->current_token;
    expr->bool.value = current_token_is(p, TRUE);
    return expr;
}

struct expression *parse_grouped_expression(struct parser *p) {
    next_token(p);
    
    struct expression *expr = parse_expression(p, LOWEST);

    if (!expect_next_token(p, RPAREN)) {
        free(expr);
        return NULL;
    }

    return expr;
}

struct block_statement *parse_block_statement(struct parser *p) {
    struct block_statement *b = malloc(sizeof (struct block_statement));
    if (!b) {
        return NULL;
    }

    b->cap = 16;
    b->size = 0;

    b->statements = malloc(b->cap * sizeof (struct statement));
    next_token(p);

    while (!current_token_is(p, RBRACE) && !current_token_is(p, EOF)) {
        struct statement s;
        if (parse_statement(p, &s) > -1) {
            b->statements[b->size++] = s;

            if (b->size >= b->cap) {
                b->cap *= 2;
                b->statements = realloc(b->statements, b->cap * sizeof (struct statement));
            }
        }
        next_token(p);
    }

    return b;
}

struct expression *parse_if_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_IF;
    expr->ifelse.token = p->current_token;

    if (!expect_next_token(p, LPAREN)) {
        free(expr);
        return NULL;
    }

    next_token(p);
    expr->ifelse.condition = parse_expression(p, LOWEST);

    if (!expect_next_token(p, RPAREN)) {
        free(expr->ifelse.condition);
        free(expr);
        return NULL;
    }

    if (!expect_next_token(p, LBRACE)) {
        free(expr->ifelse.condition);
        free(expr);
        return NULL;
    }

    expr->ifelse.consequence = parse_block_statement(p);

    if (next_token_is(p, ELSE)) {
        next_token(p);

        if (!expect_next_token(p, LBRACE)) {
            free(expr->ifelse.consequence);
            free(expr->ifelse.condition);
            free(expr);
            return NULL;
        }

        expr->ifelse.alternative = parse_block_statement(p);
    }

    return expr;
}

struct identifier_list parse_function_parameters(struct parser *p) {
    struct identifier_list params = {
        .size = 0,
        .cap = 4,
    };
    params.values = malloc(sizeof (struct identifier) * params.cap);
    if (!params.values) {
        return params;
    }

    if (next_token_is(p, RPAREN)) {
        next_token(p);
        return params;
    }

    next_token(p);
    struct identifier i;
    i.token = p->current_token;
    strcpy(i.value, i.token.literal);
    params.values[params.size++] = i;

    while (next_token_is(p, COMMA)) {
        next_token(p);
        next_token(p);

        struct identifier i;
        i.token = p->current_token;
        strcpy(i.value, i.token.literal);
        params.values[params.size++] = i;

        if (params.size >= params.cap) {
            params.cap *= 2;
            params.values = realloc(params.values, params.cap * sizeof (struct identifier));
        }
    }

    if (!expect_next_token(p, RPAREN)) {
        free(params.values);
        return params;
    }

    return params;
}

struct expression *parse_function_literal(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_FUNCTION;
    expr->function.token = p->current_token;

    if (!expect_next_token(p, LPAREN)) {
        free(expr);
        return NULL;
    }

    expr->function.parameters = parse_function_parameters(p);
    if (!expect_next_token(p, LBRACE)) {
        free(expr);
        return NULL;
    }

    expr->function.body = parse_block_statement(p);
    return expr;
}

static struct expression *parse_expression(struct parser *p, int precedence) {
    struct expression *left;
    switch (p->current_token.type) {
        case IDENT: 
            left = parse_identifier_expression(p); 
            break;
        case INT: 
            left = parse_int_expression(p); 
            break;
        case BANG:
        case MINUS: 
            left = parse_prefix_expression(p);
             break;
        case TRUE:
        case FALSE: 
            left = parse_boolean_expression(p); 
            break;
        case LPAREN:
            left = parse_grouped_expression(p);
            break;
        case IF:
            left = parse_if_expression(p);
        break; 
        case FUNCTION:
            left = parse_function_literal(p);
        break;   
        default: 
            sprintf(p->error_messages[p->errors++], "no prefix parse function found for %s", token_to_str(p->current_token.type));
            return NULL;
        break;
    }

    while (!next_token_is(p, SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        enum token_type type = p->next_token.type;
        if (type == PLUS || type == MINUS || type == ASTERISK || type == SLASH || type == EQ || type == NOT_EQ || type == LT || type == GT) {
            next_token(p);
            left = parse_infix_expression(p, left);
        } else if (type == LPAREN) {
            next_token(p);
            left = parse_call_expression(p, left);
        } else {
            return left;
        }
    }

    return left;
}

static enum precedence get_token_precedence(struct token t) {
    switch (t.type) {
        case EQ: return EQUALS;
        case NOT_EQ: return EQUALS;
        case LT: return LESSGREATER;
        case GT: return LESSGREATER;
        case PLUS: return SUM;
        case MINUS: return SUM;
        case SLASH: return PRODUCT;
        case ASTERISK: return PRODUCT;
        case LPAREN: return CALL;
        default: return LOWEST;
    }

    return LOWEST;
};

static int parse_expression_statement(struct parser *p, struct statement *s) {
    s->type = STMT_EXPR;
    s->token = p->current_token;
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, SEMICOLON)) {
        next_token(p);
    } 

    return 1;
}

static int parse_statement(struct parser *p, struct statement *s) {
    switch (p->current_token.type) {
        case LET: return parse_let_statement(p, s); break;
        case RETURN: return parse_return_statement(p, s); break;
        default: return parse_expression_statement(p, s); break;
    }
  
   return -1;
}

struct program parse_program(struct parser *parser) {
    struct program prog = {
        .size = 0,
        .cap = 32, 
        .statements = malloc(sizeof (struct statement) * 32),
    };

    // handle malloc failures
    if (!prog.statements) {
        return prog;
    }

    struct statement s;
    while (parser->current_token.type != EOF) {
        
        // if an error occured, skip token & continue
        if (parse_statement(parser, &s) == -1) {
            next_token(parser);
            continue;
        }
        
        prog.statements[prog.size++] = s;

        // double program capacity if needed
        if (prog.size >= prog.cap) {
            prog.cap *= 2;
            prog.statements = realloc(prog.statements, sizeof (struct statement) * prog.cap);
        }

        next_token(parser);        
    }

    return prog;
}

static void let_statement_to_str(char *str, struct statement *stmt) {    
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    strcat(str, stmt->name.value);
    strcat(str, " = ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

static void return_statement_to_str(char *str, struct statement *stmt) {
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

static void  statement_to_str(char *str, struct statement *stmt) {
    switch (stmt->type) {
        case STMT_LET: return let_statement_to_str(str, stmt); break;
        case STMT_RETURN: return return_statement_to_str(str, stmt); break;
        case STMT_EXPR: return expression_to_str(str, stmt->value); break;
    }

    // TODO: Signal error
}

static void block_statement_to_str(char *str, struct block_statement *b) {
    for (int i=0; i < b->size; i++) {
        statement_to_str(str, &b->statements[i]);
    }
}

static void expression_to_str(char *str, struct expression *expr) {
    switch (expr->type) {
        case EXPR_PREFIX: 
            strcat(str, "(");
            strcat(str, expr->prefix.operator);
            expression_to_str(str, expr->prefix.right);
            strcat(str, ")");
        break;
        case EXPR_INFIX: 
            strcat(str, "(");
            expression_to_str(str, expr->infix.left);
            strcat(str, " ");
            strcat(str, expr->prefix.operator);
            strcat(str, " ");
            expression_to_str(str, expr->infix.right);
            strcat(str, ")");
        break;
        case EXPR_IDENT:
            strcat(str, expr->ident.value);
        break;
        case EXPR_BOOL:
            strcat(str, expr->bool.value ? "true" : "false");
            break;
        case EXPR_INT:
            strcat(str, expr->integer.token.literal);
        break;
        case EXPR_IF:
            strcat(str, "if ");
            expression_to_str(str, expr->ifelse.condition);
            strcat(str, " ");
            block_statement_to_str(str, expr->ifelse.consequence);
            if (expr->ifelse.alternative) {
                strcat(str, "else ");
                block_statement_to_str(str, expr->ifelse.alternative);
            }
        break;
        case EXPR_FUNCTION:
            strcat(str, expr->function.token.literal);
            strcat(str, "(");
            for (int i=0; i < expr->function.parameters.size; i++) {
                strcat(str, expr->function.parameters.values[i].token.literal);
                if (i < expr->function.parameters.size-1) {
                    strcat(str, ", ");
                }
            }
            strcat(str, ") ");
            block_statement_to_str(str, expr->function.body);
        break;
        case EXPR_CALL:
            expression_to_str(str, expr->call.function);
            strcat(str, "(");
            for (int i=0; i < expr->call.arguments.size; i++){
                expression_to_str(str, &expr->call.arguments.values[i]);
                if (i < expr->call.arguments.size-1) {
                    strcat(str, ", ");
                }
            }
            strcat(str, ")");
        break;
        default: 
            // TODO: Signal missing to_str implementation
        break;
    }

}

char *program_to_str(struct program *p) {
    // TODO: Use some kind of buffer here that dynamically grows
    char *str = malloc(256);
    if (!str) {
        return "Failed to allocate enough memory.";
    }

    *str = '\0';

    for (int i = 0; i < p->size; i++) {  
        statement_to_str(str, &p->statements[i]);
        
        if (i < p->size -1) {
            str = realloc(str, sizeof str + 256);
        }
    }    

    return str;
}

static void free_statements(struct statement *stmts, unsigned int size) {
    for (int i=0; i < size; i++) {
        free_expression(stmts[i].value);
    }
    
    free(stmts);
}

static void free_expression(struct expression *expr) {
    if (!expr) {
        return;
    }

    switch (expr->type) {
        case EXPR_INFIX:
            free_expression(expr->infix.left);
            free_expression(expr->infix.right);
        break;

        case EXPR_FUNCTION:
            free(expr->function.parameters.values);
            free_statements(expr->function.body->statements, expr->function.body->size);
            free(expr->function.body);
        break;

        case EXPR_IF:
            free_expression(expr->ifelse.condition);

            free_statements(expr->ifelse.consequence->statements, expr->ifelse.consequence->size);
            free(expr->ifelse.consequence);

            if (expr->ifelse.alternative) {
                free_statements(expr->ifelse.alternative->statements, expr->ifelse.alternative->size);
                free(expr->ifelse.alternative);
            }
        break;

        case EXPR_PREFIX: 
            free_expression(expr->prefix.right);
        break;

        case EXPR_CALL:
            free_expression(expr->call.arguments.values);
            free_expression(expr->call.function);
        break;

        default: 
        break;
    }

    free(expr);
}

void free_program(struct program *p) {
    free_statements(p->statements, p->size);
}