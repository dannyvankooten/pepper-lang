#ifndef LEXER_H
#define LEXER_H

#include "token.h"

struct lexer {
    char *input;
    unsigned int pos;
};

int gettoken(struct lexer *l, struct token *t);

extern struct lexer new_lexer(char *input);

#endif