#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "parser.h"
#include "lexer.h"
#include "util.h"


enum precedence {
    LOWEST = 1,
    ASSIGN,
    LOGICAL_OR,
    LOGICAL_AND,
    EQUALS,         // ==
    LESSGREATER,    // < or >
    SUM,            // - and +
    PRODUCT,        // * and /
    PREFIX,         // - or !x
    CALL,           // fn()
    INDEX,          // array[i]
};

static struct expression *parse_expression(struct parser *p, enum precedence precedence);
static int parse_statement(struct parser *p, struct statement *s);
static void expression_to_str(char *str, const struct expression *expr);
static void free_expression(struct expression *expr);
static enum operator parse_operator(enum token_type t);
void free_parser(struct parser* p);

static 
enum precedence get_token_precedence(const struct token t) {
    switch (t.type) {
        default: 
            return LOWEST;
        break;
        case TOKEN_ASSIGN: 
            return ASSIGN;
        break;
        case TOKEN_OR:
            return LOGICAL_OR;
            break;
        case TOKEN_AND:
            return LOGICAL_AND;
            break;
        case TOKEN_EQ:
        case TOKEN_NOT_EQ: 
            return EQUALS;
        break;
        case TOKEN_LTE:
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_GTE:
            return LESSGREATER;
        break;
        case TOKEN_PLUS: 
        case TOKEN_MINUS: 
            return SUM;
        break;
        case TOKEN_PERCENT:
        case TOKEN_SLASH:
        case TOKEN_ASTERISK: 
            return PRODUCT;
        break;
        case TOKEN_LPAREN: 
            return CALL;
        break;
        case TOKEN_LBRACKET: 
            return INDEX;
        break;
    }
}

struct parser new_parser(struct lexer *l) {
    struct parser p = {
        .lexer = l,
        .errors = 0,
    };
    p.error_messages = NULL;

    // read two tokens so that both current_token and next_token are set
    gettoken(p.lexer, &p.current_token);
    gettoken(p.lexer, &p.next_token);
    return p;
}


static void 
add_parsing_error(struct parser *p, const char *_format, ...) 
{
    va_list args;

    // create format string with filename, line # & line pos prepended
    char format[256];
    sprintf(format, "%d:%d: %s", p->current_token.line, p->current_token.pos, _format);

    // alocate space for error message
    p->error_messages = realloc(p->error_messages, (p->errors + 1) * sizeof(char*));
    assert(p->error_messages != NULL);
    uint32_t cap = (strlen(format) + 64) * 2;
    p->error_messages[p->errors] = malloc(cap);
    char *msg = p->error_messages[p->errors++];

    va_start(args, _format);  
    vsnprintf(msg, cap, format, args);
    va_end(args);
}

static void
next_token(struct parser * p) {
    p->current_token = p->next_token;

    if (p->current_token.type != TOKEN_EOF) {
        gettoken(p->lexer, &p->next_token);
    }
}

static bool 
current_token_is(struct parser *p, enum token_type t) {
    return t == p->current_token.type;
}

static bool 
next_token_is(struct parser *p, enum token_type t) {
    return t == p->next_token.type;
}

static void skip_forward(struct parser *p) {
    // skip forward to next semicolon
    while (!current_token_is(p, TOKEN_SEMICOLON) && !current_token_is(p, TOKEN_EOF)) {
        next_token(p);
    }
}

static bool 
advance_to_next_token(struct parser *p, enum token_type t) {
    if (next_token_is(p, t)) {
        next_token(p);
        return true;
    }

    add_parsing_error(p, "Expected \"%s\", got \"%s\" instead", token_type_to_str(t), p->next_token.literal);
    skip_forward(p);

    return false;
}

static 
struct expression *make_expression(enum expression_type type, struct token tok) {
    struct expression *expr = malloc(sizeof *expr);
    if (!expr) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }

    expr->type = type;
    expr->token = tok;
    return expr;
}

static bool
parse_let_statement(struct parser *p, struct statement *stmt) {
    stmt->type = STMT_LET;
    stmt->token = p->current_token;
    stmt->value = NULL;

    if (!advance_to_next_token(p, TOKEN_IDENT)) {
        return false;
    }

    // parse identifier
    struct identifier ident = {
        .token = p->current_token,
    };
    strcpy(ident.value, p->current_token.literal);   
    stmt->name = ident;
    

    // Optional assignment
    // Test: let foo;
    if (!next_token_is(p, TOKEN_ASSIGN)) {
        return true;
    }

    // parse expression
    next_token(p); // Skip TOKEN_ASSIGN
    next_token(p);
    stmt->value = parse_expression(p, LOWEST);
    if (stmt->value && stmt->value->type == EXPR_FUNCTION) {
        strcpy(stmt->value->function.name, stmt->name.value);
    } 
    return true;
}

static
int parse_return_statement(struct parser *p, struct statement *s) {
    s->type = STMT_RETURN;
    s->token = p->current_token;
    next_token(p);
    s->value = parse_expression(p, LOWEST);
    return 1;
}

static
struct expression *parse_identifier_expression(const struct parser *p) {
    struct expression *expr = make_expression(EXPR_IDENT, p->current_token);
    expr->ident.token = expr->token;
    strcpy(expr->ident.value, p->current_token.literal);
    return expr;
}

static
struct expression *parse_string_literal(struct parser *p) {
    const uint32_t len = p->current_token.end - p->current_token.start + 1;
    struct expression *expr = (struct expression *) malloc(sizeof(*expr) + len);
    if (!expr) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }

    expr->type = EXPR_STRING;
    expr->token = p->current_token;
    expr->string = (char *) (expr + 1);

    char *dest = expr->string;
    const char *source = p->current_token.start;
    while (source != p->current_token.end) {
        // handle (some) backslash escape sequences
        if (*source == '\\') {
            switch(*(source + 1)) {
                case 'n':
                    *dest++ = '\n';
                    source += 2;
                break;
                
                case 't':
                    *dest++ = '\t';
                    source += 2;
                break;

                case '"':
                    *dest++ = '"';
                    source += 2;
                break;

                default:
                    *dest++ = *source++;
                break;
            }
        } else {
            *dest++ = *source++;
        }
    }
    *dest = '\0';
    return expr;
}

static
struct expression *parse_int_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_INT, p->current_token);
    expr->integer = 0;
    char *s = expr->token.literal;
    while (*s != '\0') {
        expr->integer = (expr->integer * 10) + (*s++ - '0');
    }
    return expr;
}

static
struct expression *parse_prefix_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_PREFIX, p->current_token); 
    expr->prefix.operator = parse_operator(p->current_token.type);
    next_token(p);
    expr->prefix.right = parse_expression(p, PREFIX);
    return expr;
}

static
struct expression_list parse_expression_list(struct parser *p, const enum token_type end_token) {
    struct expression_list list = {
        .size = 0,
        .cap = 0,
    };

    // return empty list if next token is already end token
    if (next_token_is(p, end_token)) {
        next_token(p);
        return list;
    }

    list.cap = 4;
    list.values = malloc(list.cap * sizeof **list.values);
    if (!list.values) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }
    next_token(p);
    list.values[list.size++] = parse_expression(p, LOWEST);

    while (next_token_is(p, TOKEN_COMMA)) {
        next_token(p); // skip comma
        next_token(p);

        list.values[list.size++] = parse_expression(p, LOWEST);

        // double capacity if needed
        if (list.size >= list.cap) {
            list.cap *= 2;
            list.values = realloc(list.values, list.cap * sizeof **list.values);
            assert(list.values != NULL);
        }
    }

    if (!advance_to_next_token(p, end_token)) {
        free(list.values);
        return list;
    }

    return list;
}

static
struct expression *parse_array_literal(struct parser *p) {
    struct expression *expr = make_expression(EXPR_ARRAY, p->current_token); 
    expr->array = parse_expression_list(p, TOKEN_RBRACKET);
    return expr;
}

static
struct expression *parse_slice_expression(struct parser *p, struct expression* expr) {
    expr->type = EXPR_SLICE;
    expr->slice.left = expr->index.left;
    expr->slice.start = expr->index.index;
    expr->slice.end = NULL;
    
    next_token(p); // Skip :

    if (! next_token_is(p, TOKEN_RBRACKET)) {
        next_token(p);
        expr->slice.end = parse_expression(p, LOWEST);
    }

    if (!advance_to_next_token(p, TOKEN_RBRACKET)) {
        free_expression(expr);
        return NULL;
    }

    return expr;
}

static
struct expression *parse_index_expression(struct parser *p, struct expression *left) {
    if (!advance_to_next_token(p, TOKEN_LBRACKET)) {
        return NULL;
    }

    struct expression *expr = make_expression(EXPR_INDEX, p->current_token);
    expr->index.left = left;
    expr->index.index = NULL;

    // Test: [:1]
    if (next_token_is(p, TOKEN_COLON)) {
        return parse_slice_expression(p, expr);
    }
    
    next_token(p);
    expr->index.index = parse_expression(p, LOWEST);

    // Test: [0:1]
    if (next_token_is(p, TOKEN_COLON)) {
        return parse_slice_expression(p, expr);
    }

    // For index expressions, index can not be NULL
    if (expr->index.index == NULL) {
        free_expression(expr);
        return NULL;
    }    

    if (!advance_to_next_token(p, TOKEN_RBRACKET)) {
        free_expression(expr);
        return NULL;
    }
    return expr;
}

static
struct expression *parse_call_expression(struct parser *p, struct expression *left) {
    struct expression *expr = make_expression(EXPR_CALL, p->current_token); 
    expr->call.function = left;
    expr->call.arguments = parse_expression_list(p, TOKEN_RPAREN);
    return expr;
}

static 
struct expression *parse_assignment_expression(struct parser *p, struct expression *left) {
    if (left->type != EXPR_IDENT && left->type != EXPR_INDEX) {
        add_parsing_error(p, "Invalid assignment left-hand side. Expected IDENT or INDEX, got %s.", expression_type_to_str(left->type));
        return NULL;
    }

    struct expression * expr = make_expression(EXPR_ASSIGN, p->current_token); 
    expr->assign.left = left;
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->assign.value = parse_expression(p, precedence);
    return expr;
}

static
struct expression *parse_postfix_expression(struct parser *p, struct expression *left) {
    struct expression * expr = make_expression(EXPR_POSTFIX, p->current_token);
    expr->postfix.left = left;
    next_token(p);
    expr->postfix.operator = parse_operator(p->current_token.type);
    return expr;
}

static
struct expression *parse_infix_expression(struct parser *p, struct expression *left) {
    struct expression * expr = make_expression(EXPR_INFIX, p->current_token);
    expr->infix.left = left;
    expr->infix.operator = parse_operator(p->current_token.type);
    int precedence = get_token_precedence(p->current_token);
    next_token(p);
    expr->infix.right = parse_expression(p, precedence);
    return expr;
}

static
struct expression *parse_boolean_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_BOOL, p->current_token);
    expr->boolean = current_token_is(p, TOKEN_TRUE);
    return expr;
}

static
struct expression *parse_grouped_expression(struct parser *p) {
    next_token(p);
    
    struct expression *expr = parse_expression(p, LOWEST);
    if (expr == NULL) {
        return NULL;
    }
    if (!advance_to_next_token(p, TOKEN_RPAREN)) {
        return NULL;
    }

    return expr;
}

static struct block_statement* 
create_block_statement(unsigned cap) {
    struct block_statement *b = (struct block_statement *) malloc(sizeof *b);
    assert(b != NULL);
    b->cap = cap;
    b->size = 0;
    b->statements = malloc(b->cap * sizeof (*b->statements));
    assert(b->statements != NULL);
    return b;
}

static void
add_statement_to_block(struct block_statement *b, struct statement* s) {
    if (b->size + 1 == b->cap) {
        b->cap *= 2;
        b->statements = realloc(b->statements, b->cap * sizeof *b->statements);
        assert(b->statements != NULL);
    }

    b->statements[b->size++] = *s;
}

static
struct block_statement *parse_block_statement(struct parser *p) {
    struct block_statement *b = create_block_statement(4);
    next_token(p); // LBRACE

    while (!current_token_is(p, TOKEN_RBRACE) && !current_token_is(p, TOKEN_EOF)) {
        struct statement s;
        if (parse_statement(p, &s) > -1) {
            add_statement_to_block(b, &s); 
        }

        // optional semicolon after every statement
        if (next_token_is(p, TOKEN_SEMICOLON)) {
            next_token(p);
        }

        next_token(p);
    }

    return b;
}

static
struct expression *parse_for_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_FOR, p->current_token);
    expr->for_loop.init.value = NULL;
    expr->for_loop.init.type = STMT_EXPR;
    expr->for_loop.inc.value = NULL;
    expr->for_loop.inc.type = STMT_EXPR;
    expr->for_loop.condition = NULL;

    if (!advance_to_next_token(p, TOKEN_LPAREN)) {
        free_expression(expr);
        return NULL;
    }
    
    if (!next_token_is(p, TOKEN_SEMICOLON)) {
        next_token(p);
        parse_statement(p, &expr->for_loop.init);
    }
    if (!advance_to_next_token(p, TOKEN_SEMICOLON)) {
        free_expression(expr);
        return NULL;
    }
            
    if (!next_token_is(p, TOKEN_SEMICOLON)) {
        next_token(p);
        expr->for_loop.condition = parse_expression(p, LOWEST);
    }
    if (!advance_to_next_token(p, TOKEN_SEMICOLON)) {
        free_expression(expr);
        return NULL;
    }
    
    if (!next_token_is(p, TOKEN_RPAREN)) {
        next_token(p);
        parse_statement(p, &expr->for_loop.inc);
    }
    if (!advance_to_next_token(p, TOKEN_RPAREN)) {
        free_expression(expr);
        return NULL;
    }
    if (!advance_to_next_token(p, TOKEN_LBRACE)) {
        free_expression(expr);
        return NULL;
    }

    expr->for_loop.body = parse_block_statement(p);
    return expr;
}

static
struct expression *parse_while_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_WHILE, p->current_token);
    if (!advance_to_next_token(p, TOKEN_LPAREN)) {
        free_expression(expr);
        return NULL;
    }

    next_token(p);
    expr->while_loop.condition = parse_expression(p, LOWEST);

    if (!advance_to_next_token(p, TOKEN_RPAREN)) {
        free_expression(expr);
        return NULL;
    }
    if (!advance_to_next_token(p, TOKEN_LBRACE)) {
        free_expression(expr);
        return NULL;
    }
    expr->while_loop.body = parse_block_statement(p);
    return expr;
}

static
struct expression *parse_if_expression(struct parser *p) {
    struct expression *expr = make_expression(EXPR_IF, p->current_token); 
    if (!advance_to_next_token(p, TOKEN_LPAREN)) {
        free_expression(expr);
        return NULL;
    }

    next_token(p);
    expr->ifelse.condition = parse_expression(p, LOWEST);

    if (!advance_to_next_token(p, TOKEN_RPAREN)) {
        free_expression(expr);
        return NULL;
    }
    if (!advance_to_next_token(p, TOKEN_LBRACE)) {
        free_expression(expr);
        return NULL;
    }

    expr->ifelse.consequence = parse_block_statement(p);
    expr->ifelse.alternative = NULL;

    if (next_token_is(p, TOKEN_ELSE)) {
        next_token(p);
        
        // if this is an "else if" statement, put the if inside a block statement
        if (next_token_is(p, TOKEN_IF)) {
            next_token(p);

            struct block_statement *b = create_block_statement(1);
            parse_statement(p, &b->statements[b->size++]);
            expr->ifelse.alternative = b;
            return expr;
        }

        if (!advance_to_next_token(p, TOKEN_LBRACE)) {
            free_expression(expr);
            return NULL;
        }
        expr->ifelse.alternative = parse_block_statement(p);
    }

    return expr;
}

static
struct identifier_list parse_function_parameters(struct parser *p) {
    struct identifier_list params = {
        .size = 0,
        .cap = 0,
    };
    
    // Return immediately if no function parameters
    if (next_token_is(p, TOKEN_RPAREN)) {
        next_token(p);
        return params;
    }

    params.cap = 4;
    params.values = malloc(sizeof *params.values * params.cap);
    if (!params.values) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }

    next_token(p);
    struct identifier i;
    i.token = p->current_token;
    strcpy(i.value, i.token.literal);
    params.values[params.size++] = i;

    while (next_token_is(p, TOKEN_COMMA)) {
        next_token(p);  // Load ','
        next_token(p);  // Load argument name

        struct identifier i;
        i.token = p->current_token;
        strcpy(i.value, i.token.literal);
        params.values[params.size++] = i;

        if (params.size >= params.cap) {
            params.cap *= 2;
            params.values = realloc(params.values, params.cap * sizeof *params.values);
        }
    }

    if (!advance_to_next_token(p, TOKEN_RPAREN)) {
        // TODO: Signal error
    }
    return params;
}

static
struct expression *parse_function_literal(struct parser *p) {
    struct expression *expr = make_expression(EXPR_FUNCTION, p->current_token); 
    expr->function.body = NULL;
    if (!advance_to_next_token(p, TOKEN_LPAREN)) {
        free_expression(expr);
        return NULL;
    }
    strcpy(expr->function.name, "");
    expr->function.parameters = parse_function_parameters(p);
    if (!advance_to_next_token(p, TOKEN_LBRACE)) {
        free_expression(expr);
        return NULL;
    }
    expr->function.body = parse_block_statement(p);
    return expr;
}

static struct expression *parse_expression(struct parser *p, const enum precedence precedence) {
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
        case TOKEN_FOR:
            left = parse_for_expression(p);
        break;
        case TOKEN_WHILE: 
            left = parse_while_expression(p);
        break;
        case TOKEN_FUNCTION:
            left = parse_function_literal(p);
        break;   
        case TOKEN_STRING: 
            left = parse_string_literal(p);
        break;
        case TOKEN_LBRACKET: 
            left = parse_array_literal(p);
        break;
        // Semicolon is just an empty expression, we can safely ignore it
        case TOKEN_SEMICOLON:
            return NULL;
        break;
        default: 
            add_parsing_error(p, "Expected expression, got \"%s\".", p->current_token.literal);
            skip_forward(p);
            return NULL;
        break;
    }

    // maybe parse right (infix) expression
    while (!next_token_is(p, TOKEN_SEMICOLON) && precedence < get_token_precedence(p->next_token)) {
        enum token_type type = p->next_token.type;
        switch (type) {
            case TOKEN_ASSIGN:
                next_token(p);
                left = parse_assignment_expression(p, left);
            break;
            case TOKEN_PLUS: 
            case TOKEN_MINUS: 
            case TOKEN_ASTERISK: 
            case TOKEN_SLASH: 
            case TOKEN_EQ: 
            case TOKEN_NOT_EQ: 
            case TOKEN_LT: 
            case TOKEN_LTE:
            case TOKEN_GT: 
            case TOKEN_GTE:
            case TOKEN_AND:
            case TOKEN_OR:
            case TOKEN_PERCENT:
                next_token(p);
                // Test: foo++
                // Test: foo--
                if (left->type == EXPR_IDENT && next_token_is(p, type) && (next_token_is(p, TOKEN_PLUS) || next_token_is(p, TOKEN_MINUS))) {
                    left = parse_postfix_expression(p, left);
                } else {
                    left = parse_infix_expression(p, left);
                }
            break; 

            case TOKEN_LPAREN: 
                next_token(p);
                left = parse_call_expression(p, left);
            break; 

            case TOKEN_LBRACKET: 
                left = parse_index_expression(p, left);
            break;

            default: 
                return left;
            break;
        }
    }

    return left;
}

static
int parse_expression_statement(struct parser *p, struct statement *s) {
    s->type = STMT_EXPR;
    s->token = p->current_token;
    s->value = parse_expression(p, LOWEST);
    if (s->value == NULL) {
        return -1;
    }
    return 1;
}

static
int parse_loop_control_statement(struct parser *p, struct statement *s) {
    s->type = p->current_token.type == TOKEN_BREAK ? STMT_BREAK : STMT_CONTINUE;
    s->token = p->current_token;
    s->value = NULL;
    return 1;
}

static int 
parse_statement(struct parser *p, struct statement *s) {    
    switch (p->current_token.type) {
        case TOKEN_LET: 
            return parse_let_statement(p, s); 
        break;
        case TOKEN_RETURN: 
            return parse_return_statement(p, s); 
        break;
        case TOKEN_BREAK: 
        case TOKEN_CONTINUE:
            return parse_loop_control_statement(p, s);
        break;
        default: 
            return parse_expression_statement(p, s); 
        break;
    }
}

struct program *parse_program_str(const char *str) {
    struct lexer lexer = new_lexer(str);
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    // TODO: Do something with parser errors?
    free_parser(&parser);
    return program;
}

struct program *parse_program(struct parser *parser) {
    const int cap = 4;
    struct program *program = (struct program *) malloc(sizeof *program + cap * sizeof *program->statements);
    if (!program) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }

    #ifdef DEBUG 
        printf("\n\nProgram: %s\n", parser->lexer->input);
    #endif

    program->size = 0;
    program->cap = cap;
    program->statements = (struct statement *) (program + 1);

    while (parser->current_token.type != TOKEN_EOF) {
        struct statement s;
        
        // if an error occured, skip token & continue
        if (parse_statement(parser, &s) == -1) {
            next_token(parser);
            continue;
        }

        // semicolon is optional after each statement
        if (next_token_is(parser, TOKEN_SEMICOLON)) {
            next_token(parser);
        }
        
        // double program capacity if needed
        if (program->size == program->cap) {
            program->cap *= 2;
            program = realloc(program, sizeof (struct program) + (sizeof (struct statement) * program->cap));
            assert(program != NULL);
            program->statements = (struct statement *) (program + 1);
        }

        program->statements[program->size++] = s;
        next_token(parser);        
    }

    return program;
}

void let_statement_to_str(char *str, const struct statement *stmt) {    
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    strcat(str, stmt->name.value);
    strcat(str, " = ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

void return_statement_to_str(char *str, const struct statement *stmt) {
    strcat(str, stmt->token.literal);
    strcat(str, " ");
    expression_to_str(str, stmt->value);
    strcat(str, ";");
}

void statement_to_str(char *str, const struct statement *stmt) {
    switch (stmt->type) {
        case STMT_LET: let_statement_to_str(str, stmt); break;
        case STMT_RETURN: return_statement_to_str(str, stmt); break;
        case STMT_EXPR: expression_to_str(str, stmt->value); break;

        case STMT_CONTINUE: 
        case STMT_BREAK: 
            strcat(str, stmt->token.literal);
            strcat(str, ";");
        break;
    }
}

void block_statement_to_str(char *str, const struct block_statement *b) {
    for (unsigned i=0; i < b->size; i++) {
        statement_to_str(str, &b->statements[i]);
    }
}

void identifier_list_to_str(char *str, const struct identifier_list *identifiers) {
    for (unsigned i=0; i < identifiers->size; i++) {
        if (i > 0) {
            strcat(str, ", ");
        }
        strcat(str, identifiers->values[i].value);
    }
}

void expression_to_str(char *str, const struct expression *expr) {
    switch (expr->type) {
        case EXPR_PREFIX: 
            strcat(str, "(");
            strcat(str, expr->token.literal);
            expression_to_str(str, expr->prefix.right);
            strcat(str, ")");
        break;

        case EXPR_POSTFIX: 
            strcat(str, "(");
            expression_to_str(str, expr->postfix.left);
            strcat(str, expr->token.literal);
            strcat(str, ")");
        break;

        case EXPR_INFIX: 
            strcat(str, "(");
            expression_to_str(str, expr->infix.left);
            strcat(str, " ");
            strcat(str, expr->token.literal);
            strcat(str, " ");
            expression_to_str(str, expr->infix.right);
            strcat(str, ")");
        break;

        case EXPR_STRING:
            strcat(str, expr->string);
        break;

        case EXPR_IDENT:
            strcat(str, expr->ident.value);
        break;

        case EXPR_BOOL:
            strcat(str, expr->boolean ? "true" : "false");
        break;

        case EXPR_INT:
            strcat(str, expr->token.literal);
        break;

        case EXPR_ASSIGN:
            strcat(str, expr->ident.value);
            strcat(str, " = ");
            expression_to_str(str, expr->assign.value);
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

        case EXPR_WHILE: 
            strcat(str, "while ");
            expression_to_str(str, expr->while_loop.condition);
            strcat(str, " ");
            block_statement_to_str(str, expr->while_loop.body);
        break;

        case EXPR_FOR: 
            strcat(str, "for (");
            statement_to_str(str, &expr->for_loop.init);
            strcat(str, "; ");
            expression_to_str(str, expr->for_loop.condition);
            strcat(str, "; ");
            statement_to_str(str, &expr->for_loop.inc);
            strcat(str, ") {");
            block_statement_to_str(str, expr->for_loop.body);
            strcat(str, "}");
        break;

        case EXPR_FUNCTION:
            strcat(str, expr->token.literal);

            if (strlen(expr->function.name)) {
                strcat(str, "<");
                strcat(str, expr->function.name);
                strcat(str, ">");
            }
            strcat(str, "(");
            identifier_list_to_str(str, &expr->function.parameters);
            strcat(str, ") ");
            block_statement_to_str(str, expr->function.body);
        break;

        case EXPR_CALL:
            expression_to_str(str, expr->call.function);
            strcat(str, "(");
            for (unsigned i=0; i < expr->call.arguments.size; i++){
                expression_to_str(str, expr->call.arguments.values[i]);
                if (i < (expr->call.arguments.size - 1)) {
                    strcat(str, ", ");
                }
            }
            strcat(str, ")");
        break;

        case EXPR_ARRAY: 
            strcat(str, "[");
            for (unsigned i=0; i < expr->array.size; i++) {
                expression_to_str(str, expr->array.values[i]);

                if (i < (expr->array.size - 1)) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "]");
        break;

        case EXPR_INDEX: 
            strcat(str, "(");
            expression_to_str(str, expr->index.left);
            strcat(str, "[");
            expression_to_str(str, expr->index.index);
            strcat(str, "])");
        break;

        case EXPR_SLICE: 
            strcat(str, "(");
            expression_to_str(str, expr->slice.left);
            strcat(str, "[");
            expression_to_str(str, expr->slice.start);
            strcat(str, ":");
            expression_to_str(str, expr->slice.end);
            strcat(str, "])");
        break;
    }

}

char *program_to_str(const struct program *p) {
    char *str = malloc(BUFSIZ);
    if (!str) {
        err(EXIT_FAILURE, "OUT OF MEMORY");
    }
    *str = '\0';

    for (uint32_t i = 0; i < p->size; i++) {  
        statement_to_str(str, &p->statements[i]);
    }    

    return str;
}

enum operator parse_operator(enum token_type t) {
    switch (t) {
        case TOKEN_BANG: return OP_NEGATE; break;
        case TOKEN_MINUS: return OP_SUBTRACT; break;
        case TOKEN_PLUS: return OP_ADD; break;
        case TOKEN_ASTERISK: return OP_MULTIPLY; break;
        case TOKEN_GT: return OP_GT; break;
        case TOKEN_GTE: return OP_GTE; break;
        case TOKEN_LT: return OP_LT; break;
        case TOKEN_LTE: return OP_LTE; break;
        case TOKEN_EQ: return OP_EQ; break;
        case TOKEN_NOT_EQ: return OP_NOT_EQ; break;
        case TOKEN_SLASH: return OP_DIVIDE; break;
        case TOKEN_AND: return OP_AND; break;
        case TOKEN_OR: return OP_OR; break;
        case TOKEN_PERCENT: return OP_MODULO; break;
        default: return OP_UNKNOWN; break;
    }
}

char *operator_to_str(enum operator operator) {
    switch (operator) {
        case OP_ADD: return "+"; break;
        case OP_SUBTRACT: return "-"; break;
        case OP_MULTIPLY: return "*"; break;
        case OP_DIVIDE: return "/"; break;
        case OP_NEGATE: return "!"; break;
        case OP_NOT_EQ: return "!="; break;
        case OP_EQ: return "=="; break;
        case OP_LT: return "<"; break;
        case OP_LTE: return "<="; break;
        case OP_GT: return ">"; break;
        case OP_GTE: return ">="; break;
        case OP_AND: return "&&"; break;
        case OP_OR: return "||"; break;
        case OP_MODULO: return "%"; break;
        case OP_UNKNOWN:
        default: return "???"; break;
    }
}

const char* 
expression_type_to_str(enum expression_type t) {
    const char *names[] = {
        "INFIX",
        "PREFIX",
        "POSTFIX",
        "INT",
        "IDENT",
        "BOOL",
        "IF",
        "FUNCTION",
        "CALL",
        "STRING",
        "ARRAY",
        "INDEX",
        "SLICE",
        "FOR",
        "WHILE",
        "ASSIGN",
    };
    return names[t];
}

void free_statements(struct statement *stmts, const uint32_t size) {
    for (uint32_t i=0; i < size; i++) {
        free_expression(stmts[i].value);
    }
}

void free_block_statement(struct block_statement *b) {
    if (b == NULL) {
        return;
    }
    free_statements(b->statements, b->size);
    free(b->statements);
    free(b);
}

void free_expression(struct expression *expr) {
    if (expr == NULL) {
        return;
    }

    switch (expr->type) {
        case EXPR_PREFIX: 
            free_expression(expr->prefix.right);
        break;

        case EXPR_POSTFIX: 
            free_expression(expr->postfix.left);
        break;

        case EXPR_INFIX:
            free_expression(expr->infix.left);
            free_expression(expr->infix.right);
        break;

        case EXPR_FUNCTION:
            free(expr->function.parameters.values);
            free_block_statement(expr->function.body);
        break;

        case EXPR_ASSIGN:
            free_expression(expr->assign.left);
            free_expression(expr->assign.value);
        break;

        case EXPR_IF:
            free_expression(expr->ifelse.condition);
            free_block_statement(expr->ifelse.consequence);
            free_block_statement(expr->ifelse.alternative);
        break;

        case EXPR_WHILE: 
            free_expression(expr->while_loop.condition);
            free_block_statement(expr->while_loop.body);
        break;

        case EXPR_FOR: 
            free_expression(expr->for_loop.init.value);            
            free_expression(expr->for_loop.condition);
            free_expression(expr->for_loop.inc.value);
            free_block_statement(expr->for_loop.body);
        break;

        case EXPR_CALL:
            for (uint32_t i=0; i < expr->call.arguments.size; i++) {
                free_expression(expr->call.arguments.values[i]);
            }
            free(expr->call.arguments.values);
            free_expression(expr->call.function);
        break;

        case EXPR_STRING:
        break;

       case EXPR_ARRAY: 
            for (uint32_t i=0; i < expr->array.size; i++) {
                free_expression(expr->array.values[i]);
            }
            free(expr->array.values);
       break;

       case EXPR_INDEX: 
            free_expression(expr->index.left);
            free_expression(expr->index.index);
       break;

        case EXPR_SLICE: 
            free_expression(expr->slice.left);
            free_expression(expr->slice.start);
            free_expression(expr->slice.end);
       break;
    
       case EXPR_INT: 
       case EXPR_IDENT: 
       case EXPR_BOOL: 
            //nothing to free
       break;
    }

    free(expr);
}

void free_parser(struct parser* p) {
    for (unsigned i=0; i < p->errors; i++) {
        free(p->error_messages[i]);
    }
    free(p->error_messages);
}

void free_program(struct program *p) {
    free_statements(p->statements, p->size);
    free(p);
}

