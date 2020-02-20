#include <stdio.h>
#include "eval.h"
#include "test_helpers.h"

struct object *test_eval(char *input)
{
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);
    struct object *obj = eval_program(program);
    free_program(program);
    return obj;
}

void test_integer_object(struct object *obj, int expected)
{
    assertf(obj->type == OBJ_INT, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_INT), object_type_to_str(obj->type));
    assertf(obj->integer.value == expected, "wrong integer value: expected %d, got %d", expected, obj->integer.value);
}

void test_boolean_object(struct object *obj, char expected)
{
    assertf(obj->type == OBJ_BOOL, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_BOOL), object_type_to_str(obj->type));
    assertf(obj->boolean.value == expected, "wrong boolean value: expected %d, got %d", expected, obj->boolean.value);
}

void test_eval_integer_expressions()
{
    struct
    {
        char *input;
        int expected;
    } tests[] = {
        {"5", 5},
        {"10", 10},
        {"-5", -5},
        {"-10", -10},
        {"5 + 5 + 5 + 5 - 10", 10},
        {"2 * 2 * 2 * 2 * 2", 32},
        {"-50 + 100 + -50", 0},
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"20 + 2 * -10", 0},
        {"50 / 2 * 2 + 10", 60},
        {"2 * (5 + 10)", 30},
        {"3 * 3 * 3 + 10", 37},
        {"3 * (3 * 3) + 10", 37},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input);
        test_integer_object(obj, tests[i].expected);
    }
}

void test_eval_boolean_expressions()
{
    struct
    {
        char *input;
        char expected;
    } tests[] = {
        {"true", 1},
        {"false", 0},
        {"1 < 2", 1},
        {"1 > 2", 0},
        {"1 < 1", 0},
        {"1 > 1", 0},
        {"1 == 1", 1},
        {"1 != 1", 0},
        {"1 == 2", 0},
        {"1 != 2", 1},
        {"true == true", 1},
        {"false == false", 1},
        {"true == false", 0},
        {"true != false", 1},
        {"false != true", 1},
        {"(1 < 2) == true", 1},
        {"(1 < 2) == false", 0},
        {"(1 > 2) == true", 0},
        {"(1 > 2) == false", 1},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input);
        test_boolean_object(obj, tests[i].expected);
    }
}

void test_bang_operator()
{
    struct
    {
        char *input;
        char expected;
    } tests[] = {
        {"!true", 0},
        {"!false", 1},
        {"!5", 0},
        {"!!true", 1},
        {"!!false", 0},
        {"!!5", 1}

    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input);
        test_boolean_object(obj, tests[i].expected);
    }
}

union object_value {
    int integer;
    char null;
    char bool;
};

void test_object(struct object *obj, enum object_type type, union object_value value)
{
    assertf(obj->type == type, "invalid object type: expected %s, got %s", object_type_to_str(type), object_type_to_str(obj->type));

    switch (obj->type)
    {
    case OBJ_BOOL:
        return test_boolean_object(obj, value.bool);
    case OBJ_INT:
        return test_integer_object(obj, value.integer);
    default:
        break;
    }
}

void test_if_else_expressions()
{
    struct
    {
        char *input;
        union object_value value;
        enum object_type type;
    } tests[] = {
        {"if (true) { 10 }", {10}, OBJ_INT},
        {"if (false) { 10 }", {0}, OBJ_NULL},
        {"if (1) { 10 }", {10}, OBJ_INT},
        {"if (1 < 2) { 10 }", {10}, OBJ_INT},
        {"if (1 > 2) { 10 }", {0}, OBJ_NULL},
        {"if (1 > 2) { 10 } else { 20 }", {20}, OBJ_INT},
        {"if (1 < 2) { 10 } else { 20 }", {10}, OBJ_INT},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

void test_return_statements()
{
    struct
    {
        char *input;
        union object_value value;
        enum object_type type;
    } tests[] = {
        {"return 10;", {10}, OBJ_INT},
        {"return 10; 9;", {10}, OBJ_INT},
        {"return 2 * 5; 9;", {10}, OBJ_INT},
        {"9; return 2 * 5; 9;", {10}, OBJ_INT},
        {"                      \
        if (10 > 1) {           \
            if (10 > 1) {       \
                return 10;      \
            }                   \
            return 1;           \
        }", {10}, OBJ_INT}
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

void test_error_handling() {

}

int main()
{
    test_eval_integer_expressions();
    test_eval_boolean_expressions();
    test_bang_operator();
    test_if_else_expressions();
    // test_return_statements();
    // test_error_handling();
    printf("\x1b[32mAll eval tests passed!\033[0m\n");
}