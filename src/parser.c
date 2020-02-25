#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"

struct expression *parse_expression(struct parser *p, int precedence);
int parse_statement(struct parser *p, struct statement *s);
void expression_to_str(char *str, struct expression *expr);
void free_expression(struct expression *expr);

enum precedence get_token_precedence(struct token t) {
    switch (t.type) {
        case TOKEN_EQ: return EQUALS;
        case TOKEN_NOT_EQ: return EQUALS;
        case TOKEN_LT: return LESSGREATER;
        case TOKEN_GT: return LESSGREATER;
        case TOKEN_PLUS: return SUM;
        case TOKEN_MINUS: return SUM;
        case TOKEN_SLASH: return PRODUCT;
        case TOKEN_ASTERISK: return PRODUCT;
        case TOKEN_LPAREN: return CALL;
        default: return LOWEST;
    }

    return LOWEST;
}

void next_token(struct parser * p) {
    p->current_token = p->next_token;
    gettoken(p->lexer, &p->next_token);
}

struct parser new_parser(struct lexer *l) {
    struct parser p = {
        .lexer = l,
    };
   
    // read two tokens so that both current_token and next_token are set
    next_token(&p);
    next_token(&p);
    return p;
}

int current_token_is(struct parser *p, enum token_type t) {
    return t == p->current_token.type;
}

int next_token_is(struct parser *p, enum token_type t) {
    return t == p->next_token.type;
}

int expect_next_token(struct parser *p, enum token_type t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return 1;
    }

    sprintf(p->error_messages[p->errors++], "expected next token to be %s, got %s instead", token_to_str(t), token_to_str(p->next_token.type));
    return 0;
}

int parse_let_statement(struct parser *p, struct statement *s) {
    s->type = STMT_LET;
    s->token = p->current_token;

    if (!expect_next_token(p, TOKEN_IDENT)) {
        return -1;
    }

    // parse identifier
    struct identifier id = {
        .token = p->current_token,
    };
    strncpy(id.value, p->current_token.literal, MAX_IDENT_LENGTH);
    s->name = id;

    if (!expect_next_token(p, TOKEN_ASSIGN)) {
        return -1;
    }

    // parse expression
    next_token(p);
    s->value = parse_expression(p, LOWEST);
    if (next_token_is(p, TOKEN_SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

int parse_return_statement(struct parser *p, struct statement *s) {
    s->type = STMT_RETURN;
    s->token = p->current_token;

    // parse expression
    next_token(p);
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, TOKEN_SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

struct expression *parse_identifier_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_IDENT;
    expr->ident.token = p->current_token;
    strncpy(expr->ident.value, p->current_token.literal, MAX_IDENT_LENGTH);
    return expr;
}

struct expression *parse_int_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_INT;
    expr->token = p->current_token;
    expr->integer = atoi(p->current_token.literal);
    return expr;
}

struct expression *parse_prefix_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_PREFIX;
    expr->token = p->current_token;
    strncpy(expr->prefix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    next_token(p);
    expr->prefix.right = parse_expression(p, PREFIX);
    return expr;
}

struct expression_list parse_call_arguments(struct parser *p) {
    // struct expression_list *list = malloc(sizeof (struct expression_list));
    // if (!list) {
    //     errx(EXIT_FAILURE, "out of memory");
    // }
    struct expression_list list = {
        .size = 0,
        .cap = 0,
    };

    if (next_token_is(p, TOKEN_RPAREN)) {
        next_token(p);
        return list;
    }

    // allocate memory here, so we do not need an alloc for calls without any arguments
    list.cap = 4;
    list.values = malloc(list.cap * sizeof *list.values);
    if (!list.values) {
        errx(EXIT_FAILURE, "out of memory");
    }
    next_token(p);
    list.values[list.size++] = parse_expression(p, LOWEST);

    while (next_token_is(p, TOKEN_COMMA)) {
        next_token(p);
        next_token(p);

        list.values[list.size++] = parse_expression(p, LOWEST);

        // double capacity if needed
        if (list.size >= list.cap) {
            list.cap *= 2;
            list.values = realloc(list.values, list.cap * sizeof *list.values);
        }
    }

    if (!expect_next_token(p, TOKEN_RPAREN)) {
        free(list.values);
        return list;
    }

    return list;
}

struct expression *parse_call_expression(struct parser *p, struct expression *left) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }
    expr->type = EXPR_CALL;
    expr->token = p->current_token;
    expr->call.function = left;
    expr->call.arguments = parse_call_arguments(p);
    return expr;
}

struct expression *parse_infix_expression(struct parser *p, struct expression *left) {
    struct expression * expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_INFIX;
    expr->token = p->current_token;
    expr->infix.left = left;
    strncpy(expr->infix.operator, p->current_token.literal, MAX_OPERATOR_LENGTH);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->infix.right = parse_expression(p, precedence);
    return expr;
}

struct expression *parse_boolean_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_BOOL;
    expr->token = p->current_token;
    expr->bool = current_token_is(p, TOKEN_TRUE);
    return expr;
}

struct expression *parse_grouped_expression(struct parser *p) {
    next_token(p);
    
    struct expression *expr = parse_expression(p, LOWEST);

    if (!expect_next_token(p, TOKEN_RPAREN)) {
        free(expr);
        return NULL;
    }

    return expr;
}

struct block_statement *parse_block_statement(struct parser *p) {
    struct block_statement *b = malloc(sizeof *b);
    if (!b) {
        errx(EXIT_FAILURE, "out of memory");
    }

    b->cap = 16;
    b->size = 0;
    b->statements = malloc(b->cap * sizeof (struct statement));
    if (!b->statements) {
        errx(EXIT_FAILURE, "out of memory");
    }
    next_token(p);

    while (!current_token_is(p, TOKEN_RBRACE) && !current_token_is(p, TOKEN_EOF)) {
        struct statement s;
        if (parse_statement(p, &s) > -1) {
            b->statements[b->size++] = s;

            if (b->size >= b->cap) {
                b->cap *= 2;
                b->statements = realloc(b->statements, b->cap * sizeof *b->statements);
            }
        }
        next_token(p);
    }

    return b;
}

struct expression *parse_if_expression(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_IF;
    expr->token = p->current_token;

    if (!expect_next_token(p, TOKEN_LPAREN)) {
        free(expr);
        return NULL;
    }

    next_token(p);
    expr->ifelse.condition = parse_expression(p, LOWEST);

    if (!expect_next_token(p, TOKEN_RPAREN)) {
        free(expr->ifelse.condition);
        free(expr);
        return NULL;
    }

    if (!expect_next_token(p, TOKEN_LBRACE)) {
        free(expr->ifelse.condition);
        free(expr);
        return NULL;
    }

    expr->ifelse.consequence = parse_block_statement(p);

    if (next_token_is(p, TOKEN_ELSE)) {
        next_token(p);

        if (!expect_next_token(p, TOKEN_LBRACE)) {
            free(expr->ifelse.consequence);
            free(expr->ifelse.condition);
            free(expr);
            return NULL;
        }

        expr->ifelse.alternative = parse_block_statement(p);
    } else {
        expr->ifelse.alternative = NULL;
    }

    return expr;
}

struct identifier_list parse_function_parameters(struct parser *p) {
    struct identifier_list params = {
        .size = 0,
        .cap = 4,
    };
    params.values = malloc(sizeof *params.values * params.cap);
    if (!params.values) {
        errx(EXIT_FAILURE, "out of memory");
    }

    if (next_token_is(p, TOKEN_RPAREN)) {
        next_token(p);
        return params;
    }

    next_token(p);
    struct identifier i;
    i.token = p->current_token;
    strncpy(i.value, i.token.literal, MAX_IDENT_LENGTH);
    params.values[params.size++] = i;

    while (next_token_is(p, TOKEN_COMMA)) {
        next_token(p);
        next_token(p);

        struct identifier i;
        i.token = p->current_token;
        strncpy(i.value, i.token.literal, MAX_IDENT_LENGTH);
        params.values[params.size++] = i;

        if (params.size >= params.cap) {
            params.cap *= 2;
            params.values = realloc(params.values, params.cap * sizeof *params.values);
        }
    }

    if (!expect_next_token(p, TOKEN_RPAREN)) {
        free(params.values);
        return params;
    }

    return params;
}

struct expression *parse_function_literal(struct parser *p) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        errx(EXIT_FAILURE, "out of memory");
    }

    expr->type = EXPR_FUNCTION;
    expr->token = p->current_token;

    if (!expect_next_token(p, TOKEN_LPAREN)) {
        free(expr);
        return NULL;
    }

    expr->function.parameters = parse_function_parameters(p);
    if (!expect_next_token(p, TOKEN_LBRACE)) {
        free(expr);
        return NULL;
    }

    expr->function.body = parse_block_statement(p);
    return expr;
}

struct expression *parse_expression(struct parser *p, int precedence) {
    struct expression *left;
    switch (p->current_token.type) {
        case TOKEN_IDENT: 
            left = parse_identifier_expression(p); 
            break;
        case TOKEN_INT: 
            left = parse_int_expression(p); 
            break;
        case TOKEN_BANG:
        case TOKEN_MINUS: 
            left = parse_prefix_expression(p);
             break;
        case TOKEN_TRUE:
        case TOKEN_FALSE: 
            left = parse_boolean_expression(p); 
            break;
        case TOKEN_LPAREN:
            left = parse_grouped_expression(p);
            break;
        case TOKEN_IF:
            left = parse_if_expression(p);
        break; 
        case TOKEN_FUNCTION:
            left = parse_function_literal(p);
        break;   
        default: 
            sprintf(p->error_messages[p->errors++], "no prefix parse function found for %s", token_to_str(p->current_token.type));
            return NULL;
        break;
    }

    while (!next_token_is(p, TOKEN_SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        enum token_type type = p->next_token.type;
        if (type == TOKEN_PLUS || type == TOKEN_MINUS || type == TOKEN_ASTERISK || type == TOKEN_SLASH || type == TOKEN_EQ || type == TOKEN_NOT_EQ || type == TOKEN_LT || type == TOKEN_GT) {
            next_token(p);
            left = parse_infix_expression(p, left);
        } else if (type == TOKEN_LPAREN) {
            next_token(p);
            left = parse_call_expression(p, left);
        } else {
            return left;
        }
    }

    return left;
}

int parse_expression_statement(struct parser *p, struct statement *s) {
    s->type = STMT_EXPR;
    s->token = p->current_token;
    s->value = parse_expression(p, LOWEST);

    if (next_token_is(p, TOKEN_SEMICOLON)) {
        next_token(p);
    } 

    return 1;
}

int parse_statement(struct parser *p, struct statement *s) {
    switch (p->current_token.type) {
        case TOKEN_LET: return parse_let_statement(p, s); break;
        case TOKEN_RETURN: return parse_return_statement(p, s); break;
        default: return parse_expression_statement(p, s); break;
    }
  
   return -1;
}

struct program *parse_program(struct parser *parser) {
    struct program *program = malloc(sizeof *program);
    if (!program) {
        errx(EXIT_FAILURE, "out of memory");
    }

    program->size = 0;
    program->cap = 32;
    program->statements = malloc(program->cap * sizeof *program->statements);
    if (!program->statements) {
        errx(EXIT_FAILURE, "out of memory");
    }

    struct statement s;
    while (parser->current_token.type != TOKEN_EOF) {
        
        // if an error occured, skip token & continue
        if (parse_statement(parser, &s) == -1) {
            next_token(parser);
            continue;
        }
        
        program->statements[program->size++] = s;

        // double program capacity if needed
        if (program->size >= program->cap) {
            program->cap *= 2;
            program->statements = realloc(program->statements, sizeof *program->statements * program->cap);
        }

        next_token(parser);        
    }

    return program;
}

void let_statement_to_str(char *str, struct statement *stmt) {    
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    strcat(str, stmt->name.value);
    strcat(str, " = ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

void return_statement_to_str(char *str, struct statement *stmt) {
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

void statement_to_str(char *str, struct statement *stmt) {
    switch (stmt->type) {
        case STMT_LET: let_statement_to_str(str, stmt); break;
        case STMT_RETURN: return_statement_to_str(str, stmt); break;
        case STMT_EXPR: expression_to_str(str, stmt->value); break;
    }
}

void block_statement_to_str(char *str, struct block_statement *b) {
    for (int i=0; i < b->size; i++) {
        statement_to_str(str, &b->statements[i]);
    }
}

void identifier_list_to_str(char *str, struct identifier_list *identifiers) {
    for (int i=0; i < identifiers->size; i++) {
        strcat(str, identifiers->values[i].value);
        if (i < (identifiers->size - 1)) {
            strcat(str, ", ");
        }
    }
}

void expression_to_str(char *str, struct expression *expr) {
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
            strcat(str, expr->bool ? "true" : "false");
            break;
        case EXPR_INT:
            strcat(str, expr->token.literal);
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
            strcat(str, expr->token.literal);
            strcat(str, "(");
            identifier_list_to_str(str, &expr->function.parameters);
            strcat(str, ") ");
            block_statement_to_str(str, expr->function.body);
        break;
        case EXPR_CALL:
            expression_to_str(str, expr->call.function);
            strcat(str, "(");
            for (int i=0; i < expr->call.arguments.size; i++){
                expression_to_str(str, expr->call.arguments.values[i]);
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
        errx(EXIT_FAILURE, "out of memory");
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

void free_statements(struct statement *stmts, unsigned int size) {
    for (int i=0; i < size; i++) {
        free_expression(stmts[i].value);
    }
    
    free(stmts);
}

void free_expression(struct expression *expr) {
    if (expr == NULL) {
        return;
    }

    switch (expr->type) {
        case EXPR_PREFIX: 
            free_expression(expr->prefix.right);
        break;

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

        case EXPR_CALL:
            for (int i=0; i < expr->call.arguments.size; i++) {
                free_expression(expr->call.arguments.values[i]);
            }
            free(expr->call.arguments.values);
            //free(expr->call.arguments);
            free_expression(expr->call.function);
        break;

        default: 
        break;
    }

    free(expr);
}

void free_program(struct program *p) {
    free_statements(p->statements, p->size);
    free(p);
}