#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define assertf(assertion, fmt, ...) _assertf(assertion, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define TESTNAME(v) strcpy(current_test, v);
#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

char current_test[256] = {'\0'};

void _assertf(int assertion, const char filename[64], const int line, const char function_name[64], char *format, ...)
{
    if (assertion)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    if (strlen(current_test) > 0) {
        printf("%s:%d:%s failed: ", filename, line, current_test);
    } else {
        printf("%s:%d:%s failed: ", filename, line, function_name);
    }
    vprintf(format, args);
    va_end(args);
    printf("\n");
    exit(1);
}
