#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

void abortf(char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    exit(1);
}

void assertf(int assertion, char *format, ...)
{
    if (assertion)
    {
        return;
    }

    va_list args;
    va_start(args, format);

    vprintf(format, args);
    va_end(args);
    printf("\n");
    exit(1);
}
