#include <stdio.h>
#include "eval.h"
#include "test_helpers.h"

struct object test_eval(char *input) {
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    struct program program = parse_program(&parser);
    struct object obj = eval_program(&program);
    free_program(&program);
    return obj;
}

void test_integer_object(struct object obj, int expected) {
    assertf(obj.type == OBJ_INT, "wrong object type: expected %d, got %d", OBJ_INT, obj.type);
    assertf(obj.integer.value == expected, "wrong integer value: expected %d, got %d", expected, obj.integer.value);
}

void test_boolean_object(struct object obj, char expected) {
    assertf(obj.type == OBJ_BOOL, "wrong object type: expected %d, got %d", OBJ_BOOL, obj.type);
    assertf(obj.boolean.value == expected, "wrong boolean value: expected %d, got %d", expected, obj.boolean.value);
}

void test_eval_integer_expressions() {
    struct {
        char *input;
        int expected;
    } tests[] = {
        {"5", 5},
        {"10", 10}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = test_eval(tests[i].input);
        test_integer_object(obj, tests[i].expected);
    }
}

void test_eval_boolean_expressions() {
    struct {
        char *input;
        char expected;
    } tests[] = {
        {"true", 1},
        {"false", 0}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = test_eval(tests[i].input);
        test_boolean_object(obj, tests[i].expected);
    }
}


int main() {
    test_eval_integer_expressions();
    test_eval_boolean_expressions();
    printf("\x1b[32mAll eval tests passed!\033[0m\n");
}