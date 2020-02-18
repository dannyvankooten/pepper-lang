#include "lexer.h"
#include <stdlib.h>

typedef struct expression {

} expression;

typedef struct identifier {
    token token; // token.IDENT
    char value[1048];
} identifier;

typedef struct statement {
    token token; // token.LET
    identifier name;
    expression value;
} statement;

typedef struct program {
   // statement * statements[1024];
    statement statements[1024];
    unsigned int size;
} program;

typedef struct parser {
    lexer * lexer;
    token current_token;
    token next_token;
} parser;

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

    return 0;
}


int parse_let_statement(parser *p, statement *s) {
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

    while (!next_token_is(p, SEMICOLON)) {
        next_token(p);
    }

    return 1;
}

int parse_statement(parser *p, statement *s) {
   if (p->current_token.type == LET) {
       return parse_let_statement(p, s);
   }
   return -1;
}

program parse_program(parser *parser) {
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
