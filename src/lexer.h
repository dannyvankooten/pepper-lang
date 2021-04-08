#pragma once

#include <stddef.h>
#include "token.h"

struct lexer {
    char *input;
    size_t pos;
};

int gettoken(struct lexer *l, struct token *t);

extern struct lexer new_lexer(char *input);