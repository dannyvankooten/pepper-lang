#include "lexer.h"
#include <stdlib.h>

enum {
    LOWEST = 1,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL            // fn()
};

typedef enum {
    EXPR_INFIX,
    EXPR_PREFIX,
    EXPR_INT,
    EXPR_IDENT,
} expression_type;

// TODO: Make better union struct for this
typedef struct expression {
    expression_type type;
    token token; // token.IDENT
    char operator[2];
    union {
        long int_value;
        char str_value[128];
    };

    struct expression *right;
    struct expression *left;
} expression;

typedef struct identifier {
    token token; // token.IDENT
    char value[32];
} identifier;

typedef struct statement {
    token token; // token.LET
    identifier name;
    expression * value;
} statement;

// TODO: Dynamically allocate statements here
typedef struct program {
    statement statements[32];
    unsigned int size;
} program;

typedef struct parser {
    lexer * lexer;
    token current_token;
    token next_token;

    unsigned errors;
    char error_messages[8][128];
} parser;

expression * parse_expression(parser *p, int precedence);
static int get_token_precedence(token t);

static void next_token(parser * p) {
    p->current_token = p->next_token;
    gettoken(p->lexer, &p->next_token);
}

static int current_token_is(parser *p, int t) {
    return t == p->current_token.type;
}

static int next_token_is(parser *p, int t) {
     return t == p->next_token.type;
}

static int expect_next_token(parser *p, int t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return 1;
    }

    sprintf(p->error_messages[p->errors++], "expected next token to be %s, got %s instead", token_to_str(t), token_to_str(p->next_token.type));
    return 0;
}

static int parse_let_statement(parser *p, statement *s) {
    s->token = p->current_token;

     if (!expect_next_token(p, IDENT)) {
        return -1;
    }

    // parse name
    identifier id = {
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

static int parse_return_statement(parser *p, statement *s) {
    s->token = p->current_token;

    next_token(p);

    // TODO: Parse expression

    while (!current_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

static int parse_identifier_expression(parser *p, expression *e) {
    e->token = p->current_token;
    e->type = EXPR_IDENT;
    strcpy(e->str_value, p->current_token.literal);
    return 1;
}

static int parse_int_expression(parser *p, expression *e) {
    e->token = p->current_token;
    e->type = EXPR_INT;
    e->int_value =  atoi(p->current_token.literal);
    // TODO: Signal errors?
    return 1;
}

static expression * parse_prefix_expression(parser *p, expression *expr) {
    expr->token = p->current_token;
    expr->type = EXPR_PREFIX;
    strncpy(expr->operator, p->current_token.literal, 2);
    next_token(p);
    expr->right = parse_expression(p, PREFIX);
    return expr;
}

static expression * parse_infix_expression(parser *p, expression *left) {
    expression * expr = malloc(sizeof (expression));
    expr->left = left;
    expr->type = EXPR_INFIX;
    expr->token = p->current_token;
    strncpy(expr->operator, p->current_token.literal, 2);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->right =  parse_expression(p, precedence);
    return expr;
}

expression * parse_expression(parser *p, int precedence) {
    expression * left = malloc(sizeof (expression));
    switch (p->current_token.type) {
        case IDENT: parse_identifier_expression(p, left); break;
        case INT: parse_int_expression(p, left); break;
        case BANG:
        case MINUS: parse_prefix_expression(p, left); break;
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

static int get_token_precedence(token t) {
    switch (t.type) {
        case EQ: return EQUALS;
        case NOT_EQ: return EQUALS;
        case LT: return LESSGREATER;
        case GT: return LESSGREATER;
        case PLUS: return SUM;
        case MINUS: return SUM;
        case SLASH: return PRODUCT;
        case ASTERISK: return PRODUCT;
    }

    return LOWEST;
};

static int parse_expression_statement(parser *p, statement *s) {
    s->token = p->current_token;
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, SEMICOLON)) {
        next_token(p);
    } 

    return 1;
}

static int parse_statement(parser *p, statement *s) {
    switch (p->current_token.type) {
        case LET: return parse_let_statement(p, s); break;
        case RETURN: return parse_return_statement(p ,s); break;
        default: return parse_expression_statement(p, s); break;
    }
  
   return -1;
}

extern program parse_program(parser *parser) {
    program prog = {
        .size = 0,
    };

    while (parser->current_token.type != EOF) {
        statement s;
        if (parse_statement(parser, &s) != -1) {
            prog.statements[prog.size++] = s;
        }

        next_token(parser);        
    }

    return prog;
}

parser new_parser(lexer *l) {
    parser p = {
        .lexer = l,
    };
   
    // read two tokens so that both current_token and next_token are set
    next_token(&p);
    next_token(&p);
    return p;
}

static char * let_statement_to_str(statement s) {
    char * str = malloc(sizeof(s.token.literal) + sizeof(s.name.token.literal) + sizeof(s.value->token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.name.token.literal);
    strcat(str, " = ");
    strcat(str, s.value->token.literal);
    strcat(str, ";");
    return str;
}

static char * return_statement_to_str(statement s) {
    char * str = malloc(sizeof(str) + sizeof(s.token.literal) + sizeof(s.value->token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.value->token.literal);
    strcat(str, ";");
    return str;
}

static char * expression_to_str(expression *expr) {
    char * str = malloc(256);

    switch (expr->type) {
        case EXPR_PREFIX: 
            sprintf(str, "(%s%s)", expr->operator, expression_to_str(expr->right)); 
        break;
        case EXPR_INFIX: 
            sprintf(str, "(%s %s %s)", expression_to_str(expr->left), expr->operator, expression_to_str(expr->right));
        break;
        case EXPR_IDENT:
            strcpy(str, expr->str_value);
        break;
        case EXPR_INT:
            strcpy(str, expr->token.literal);
        break;
    }

    return str;
}

char * program_to_str(program *p) {
    char * str = malloc(256);
    *str = '\0';

    statement s;

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