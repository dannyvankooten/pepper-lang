#include "lexer.h"
#include <stdlib.h>

#define MAX_IDENT_LENGTH 32
#define MAX_OPERATOR_LENGTH 3

enum {
    LOWEST = 1,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL            // fn()
};

enum expression_type {
    EXPR_INFIX,
    EXPR_PREFIX,
    EXPR_INT,
    EXPR_IDENT,
    EXPR_BOOL,
    EXPR_IF,
};

struct bool_expression {
    struct token token;
    char value;
};

struct identifier_expression {
    struct token token;
    char value[MAX_IDENT_LENGTH];
};

struct integer_expression {
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

struct expression {
    enum expression_type type;
    union {
        struct bool_expression bool;
        struct identifier_expression ident;
        struct integer_expression _int;
        struct prefix_expression prefix;
        struct infix_expression infix;

        // TODO: Fix this
        struct if_expression *_if;
    };
} expression;

struct statement {
    struct token token;
    struct identifier name;
    struct expression * value;
};

struct block_statement {
    struct token token;
    struct statement *statements;
};

struct if_expression {
    struct token token;
    struct expression condition;
    struct block_statement *consequence;
    struct block_statement *alternative;
};

struct program {
    struct statement * statements;
    unsigned int cap;
    unsigned int size;
};

struct parser {
    struct lexer * lexer;
    struct token current_token;
    struct token next_token;

    // TODO: allocate this dynamically
    unsigned int errors;
    char error_messages[8][128];
};

static struct expression * parse_expression(struct parser *p, int precedence);
static int get_token_precedence(struct token t);
static void next_token(struct parser * p);
static int current_token_is(struct parser *p, int t);
static int next_token_is(struct parser *p, int t) ;
static int expect_next_token(struct parser *p, int t);

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

static int current_token_is(struct parser *p, int t) {
    return t == p->current_token.type;
}

static int next_token_is(struct parser *p, int t) {
     return t == p->next_token.type;
}

static int expect_next_token(struct parser *p, int t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return 1;
    }

    sprintf(p->error_messages[p->errors++], "expected next token to be %s, got %s instead", token_to_str(t), token_to_str(p->next_token.type));
    return 0;
}

static int parse_let_statement(struct parser *p, struct statement *s) {
    s->token = p->current_token;

     if (!expect_next_token(p, IDENT)) {
        return -1;
    }

    // parse name
    struct identifier id = {
        .token = p->current_token,
    };
    strcpy(id.value, p->current_token.literal);
    s->name = id;

    if (!expect_next_token(p, ASSIGN)) {
        return -1;
    }

    // TODO: Read expression here, for now we just skip forward until semicolon

    while (!current_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static int parse_return_statement(struct parser *p, struct statement *s) {
    s->token = p->current_token;

    next_token(p);

    // TODO: Parse expression

    while (!current_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static struct expression *parse_identifier_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    expr->type = EXPR_IDENT;
    expr->ident.token = p->current_token;
    strncpy(expr->ident.value, p->current_token.literal, MAX_IDENT_LENGTH);
    return expr;
}

static struct expression *parse_int_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    expr->type = EXPR_INT;
    expr->_int.token = p->current_token;
    expr->_int.value =  atoi(p->current_token.literal);
    return expr;
}

static struct expression *parse_prefix_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
    expr->type = EXPR_PREFIX;
    expr->prefix.token = p->current_token;
    strncpy(expr->prefix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    next_token(p);
    expr->prefix.right = parse_expression(p, PREFIX);
    return expr;
}

static struct expression *parse_infix_expression(struct parser *p, struct expression *left) {
    struct expression * expr = malloc(sizeof (struct expression));
    expr->type = EXPR_INFIX;
    expr->infix.left = left;
    expr->infix.token = p->current_token;
    strncpy(expr->infix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->infix.right =  parse_expression(p, precedence);
    return expr;
}

struct expression *parse_boolean_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof (struct expression));
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
        default: 
            sprintf(p->error_messages[p->errors++], "no prefix parse function found for %s", token_to_str(p->current_token.type));
            return NULL;
        break;
    }

    while (!next_token_is(p, SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        int type = p->next_token.type;
        if (type == PLUS || type == MINUS || type == ASTERISK || type == SLASH || type == EQ || type == NOT_EQ || type == LT || type == GT) {
            next_token(p);
            left = parse_infix_expression(p, left);
        } else {
            return left;
        }
    }

    return left;
}

static int get_token_precedence(struct token t) {
    switch (t.type) {
        case EQ: return EQUALS;
        case NOT_EQ: return EQUALS;
        case LT: return LESSGREATER;
        case GT: return LESSGREATER;
        case PLUS: return SUM;
        case MINUS: return SUM;
        case SLASH: return PRODUCT;
        case ASTERISK: return PRODUCT;
        default: return LOWEST;
    }

    return LOWEST;
};

static int parse_expression_statement(struct parser *p, struct statement *s) {
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
        case RETURN: return parse_return_statement(p ,s); break;
        default: return parse_expression_statement(p, s); break;
    }
  
   return -1;
}

extern struct program parse_program(struct parser *parser) {
    struct program prog = {
        .size = 0,
        .cap = 32, 
        .statements = malloc(sizeof (struct statement) * 32),
    };

    struct statement s;
    while (parser->current_token.type != EOF) {
        // if an error occured, skip token & continue
        if (parse_statement(parser, &s) == -1) {
            next_token(parser);
            continue;
        }

        // add to program statements
        prog.statements[prog.size++] = s;

        // increase program capacity if needed (by doubling it)
        if (prog.size >= prog.cap) {
            prog.statements = realloc(prog.statements, sizeof (struct statement) * prog.cap * 2);
            prog.cap *= 2;
        }

        // keep going
        next_token(parser);        
    }

    return prog;
}

static char * let_statement_to_str(struct statement s) {
    char * str = malloc(sizeof(s.token.literal) + sizeof(s.name.token.literal) + sizeof(s.value->ident.token.literal) + 16);
    sprintf(str, "%s %s = %s;", s.token.literal, s.name.token.literal, s.value->ident.token.literal);
    return str;
}

static char * return_statement_to_str(struct statement s) {
    char * str = malloc(sizeof(s.token.literal) + sizeof(s.value->ident.token.literal) + 16);
    sprintf(str, "%s %s;", s.token.literal, s.value->ident.token.literal);
    return str;
}

static char * expression_to_str(struct expression *expr) {
    char * str = malloc(256);

    switch (expr->type) {
        case EXPR_PREFIX: 
            sprintf(str, "(%s%s)", expr->prefix.operator, expression_to_str(expr->prefix.right)); 
        break;
        case EXPR_INFIX: 
            sprintf(str, "(%s %s %s)", expression_to_str(expr->infix.left), expr->infix.operator, expression_to_str(expr->infix.right));
        break;
        case EXPR_IDENT:
            strcpy(str, expr->ident.value);
        break;
        case EXPR_BOOL:
            strcpy(str, expr->bool.value ? "true" : "false");
            break;
        case EXPR_INT:
            sprintf(str, "%d", expr->_int.value);
        break;
        case EXPR_IF:
            //sprintf(str, "if %s %s");
            // TODO
        break;
    }

    return str;
}

char * program_to_str(struct program *p) {
    char * str = malloc(256);
    *str = '\0';

    struct statement s;
    for (int i = 0; i < p->size; i++) {
        s = p->statements[i];
               
        switch (s.token.type) {
            case LET: 
                strcat(str, let_statement_to_str(s)); 
            break;
            case RETURN: 
                strcat(str, return_statement_to_str(s)); 
            break;
            default:
                strcat(str, expression_to_str(s.value));
            break;
        }
        
        if (i < p->size -1) {
            str = realloc(str, sizeof str + 256);
        }
    }    

    return str;
}