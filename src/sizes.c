#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "object.h"


int main() {
    printf("sizeof(char): %ld\n", sizeof(char));
    printf("sizeof(enum): %ld\n", sizeof(enum { A, B}));
    printf("sizeof(char*): %ld\n", sizeof(char*));
    printf("sizeof(bool): %ld\n", sizeof(bool));
    printf("sizeof(int): %ld\n", sizeof(int));
    printf("sizeof(int8_t): %ld\n", sizeof(int8_t));
    printf("sizeof(int_fast8_t): %ld\n", sizeof(int_fast8_t));
    printf("sizeof(int16_t): %ld\n", sizeof(int16_t));
    printf("sizeof(int32_t): %ld\n", sizeof(int32_t));
    printf("sizeof(int64_t): %ld\n", sizeof(int64_t));
    printf("sizeof(struct { } }): %ld\n", sizeof(struct { }));
    printf("sizeof(struct { bool, int }): %ld\n", sizeof(struct { bool a; int b; }));
    printf("sizeof(struct object): %ld\n", sizeof(struct object));



    struct test {
        union {
            int *a;
            char *b;
        } value;
    } t;

    t.value.b = malloc(sizeof(int));
    printf("%p\n", t.value.b);
    printf("%p\n", t.value);
}