#pragma once

#include <stddef.h>
#include "token.h"
#include <stdint.h>

struct lexer {
    const char *input;
    uint32_t pos;
};

int gettoken(struct lexer *l, struct token *t);

extern struct lexer new_lexer(const char *input);