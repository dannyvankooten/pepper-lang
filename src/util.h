#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

__attribute__((noreturn))
static inline void err(__attribute__((unused)) int status, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
   
    exit(EXIT_FAILURE);
}
