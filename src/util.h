#pragma once
#include <stdio.h>
#include <stdlib.h>

static inline void err(int status, char *format, ...) {
    va_list ap;
    fprintf(stderr, format, ap);
    exit(EXIT_FAILURE);
}
