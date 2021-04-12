#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static inline void err(int status, char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
   
    exit(EXIT_FAILURE);
}
