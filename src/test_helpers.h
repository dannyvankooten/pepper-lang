#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define assertf(assertion, fmt, ...) _assertf(assertion, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

void _assertf(int assertion, const char filename[64], const int line, const char function_name[64], char *format, ...)
{
    if (assertion)
    {
        return;
    }

    va_list args;
    va_start(args, format);

    printf("%s:%d:%s failed: ", filename, line, function_name);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    exit(1);
}
