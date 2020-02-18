#include "lexer.h"
#include <stdlib.h>

enum {
    LOWEST = 1,
    EQUALS,
    LESSGREATER,
    SUM,
    PRODUCT,
    PREFIX,
    CALL
};

typedef struct expression {
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
    identifier value;

    expression expression;
} statement;

typedef struct program {
    statement statements[64];
    unsigned int size;
} program;

typedef struct parser {
    lexer * lexer;
    token current_token;
    token next_token;

    unsigned errors;
    char error_messages[8][128];
} parser;

static int parse_expression(parser *p, expression *e, int precedence);
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
    strcpy(e->str_value, p->current_token.literal);
    return 1;
}

static int parse_int_expression(parser *p, expression *e) {
    e->token = p->current_token;
    e->int_value =  atoi(p->current_token.literal);
    // TODO: Signal errors?
    return 1;
}

static int parse_prefix_expression(parser *p, expression *e) {
    e->token = p->current_token;
    strncpy(e->operator, p->current_token.literal, 2);
    next_token(p);
    expression right;
    parse_expression(p, &right, PREFIX);
    e->right = &right;
    return 1;
}

static int parse_infix_expression(parser *p, expression *left, expression *e) {
    e->left = left;
    e->token = p->current_token;
    strncpy(e->operator, p->current_token.literal, 2);
    next_token(p);
    int precedence = get_token_precedence(p->current_token);
    parse_expression(p, e->right, precedence);
    return 1;
}

static int parse_expression(parser *p, expression *left, int precedence) {

    int success;

    switch (p->current_token.type) {
        case IDENT: success = parse_identifier_expression(p, left); break;
        case INT: success =  parse_int_expression(p, left); break;
        case BANG:
        case MINUS: success =  parse_prefix_expression(p, left); break;
        default: 
            sprintf(p->error_messages[p->errors++], "no prefix parse function found for %s", token_to_str(p->current_token.type));
            return -1;
        break;
    }

    if (!success) {
        return -1;
    }



    while (!next_token_is(p, SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        expression right;
        next_token(p);
        parse_infix_expression(p, left, &right);

        // TODO: Right becomes left here, recursively keep going
    }

    return 1;
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

static int parse_expression_statement(parser *p ,statement *s) {
    s->token = p->current_token;

    if (parse_expression(p, &s->expression, LOWEST) < 0) {
        sprintf(p->error_messages[p->errors++], "error parsing expression");
        return -1;
    }

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
        //statement *s = malloc(sizeof (struct statement));
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

static char * let_statement_to_str(char * str, statement s) {
    str = realloc(str, sizeof(str) + sizeof(s.token.literal) + sizeof(s.name.token.literal) + sizeof(s.value.token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.name.token.literal);
    strcat(str, " = ");
    strcat(str, s.value.token.literal);
    strcat(str, ";");
    return str;
}

static char * return_statement_to_str(char * str, statement s) {
    str = realloc(str, sizeof(str) + sizeof(s.token.literal) + sizeof(s.value.token.literal) + 16);
    strcat(str, s.token.literal);
    strcat(str, " ");
    strcat(str, s.value.token.literal);
    strcat(str, ";");
    return str;
}

char * program_to_str(program *p) {
    char * str = malloc(1);
    *str = '\0';

    statement s;
    for (int i = 0; i < p->size; i++) {
        s = p->statements[i];
        switch (s.token.type) {
            case LET: let_statement_to_str(str, s); break;
            case RETURN: return_statement_to_str(str, s); break;
        }

        if (i < p->size -1) {
            strcat(str, "\n");
        }
    }    

    return str;
}