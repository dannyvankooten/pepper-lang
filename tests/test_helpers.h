#ifndef TEST_HELPERS_H 
#define TEST_HELPERS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define assertf(assertion, fmt, ...) _assertf(assertion, fmt, ##__VA_ARGS__)
#define TEST(testname)                                         \
    if (argc < 2 || strcmp(argv[1], #testname) == 0) {         \
        printf("%s: %s ... ", __FILE__, #testname);            \
        fflush(stdout);                                        \
        testname();                                            \
        printf("\x1b[32mok\033[0m\n");  \
    }
#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

static
void _assertf(int assertion, char *format, ...)
{
    if (assertion) {
        return;
    }

    fflush(stdout);

    va_list args;
    va_start(args, format);
    printf("\033[91mfailed: ");
    vprintf(format, args);
    va_end(args);
    printf("\033[0m\n");
    exit(1);
}

#endif