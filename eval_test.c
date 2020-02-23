#include <stdio.h>
#include "eval.h"
#include "test_helpers.h"

// declared here so we can free it from other tests
struct program *program;

struct object *test_eval(char *input, unsigned char keep_prog)
{
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    program = parse_program(&parser);
    assertf(parser.errors == 0, "parser got %d errors", parser.errors);
    struct environment *env = make_environment(16);
    struct object *obj = eval_program(program, env);

    // Free'ing the program clears the identifier values, so we can't do that yet
    // Unless we copy them in identifier_list
    if (!keep_prog) {
        free_program(program);
    }
    free_environment(env);
    return obj;
}

void test_integer_object(struct object *obj, int expected)
{
    assertf(!!obj, "expected integer object, got null pointer");
    assertf(obj->type == OBJ_INT, "wrong object type: expected %s, got %s %s", object_type_to_str(OBJ_INT), object_type_to_str(obj->type), obj->error);
    assertf(obj->integer == expected, "wrong integer value: expected %d, got %d", expected, obj->integer);
    free_object(obj);
}

void test_boolean_object(struct object *obj, char expected)
{
    assertf(!!obj, "expected boolean object, got null pointer");
    assertf(obj->type == OBJ_BOOL, "wrong object type: expected %s, got %s %s", object_type_to_str(OBJ_BOOL), object_type_to_str(obj->type), obj->error);
    assertf(obj->boolean == expected, "wrong boolean value: expected %d, got %d", expected, obj->boolean);
    free_object(obj);
}

union object_value {
    int integer;
    char null;
    char bool;
    char *message;
};

void test_error_object(struct object *obj, char *expected) {
    assertf(!!obj, "expected error object, got null pointer");
    assertf(obj->type == OBJ_ERROR, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_ERROR), object_type_to_str(obj->type));
    assertf(strcmp(obj->error, expected) == 0, "invalid error message: expected %s, got %s", expected, obj->error);
    free_object(obj);
}

void test_object(struct object *obj, enum object_type type, union object_value value)
{
    switch (type)
    {
    case OBJ_BOOL:
        return test_boolean_object(obj, value.bool);
    case OBJ_INT:
        return test_integer_object(obj, value.integer);
    case OBJ_ERROR:
        return test_error_object(obj, value.message);    
    default:
        break;
    }
}

void test_eval_integer_expressions()
{
    struct
    {
        char *input;
        int expected;
    } tests[] = {
        {"5", 5},
        // {"10", 10},
        // {"-5", -5},
        // {"-10", -10},
        // {"5 + 5 + 5 + 5 - 10", 10},
        // {"2 * 2 * 2 * 2 * 2", 32},
        // {"-50 + 100 + -50", 0},
        // {"5 * 2 + 10", 20},
        // {"5 + 2 * 10", 25},
        // {"20 + 2 * -10", 0},
        // {"50 / 2 * 2 + 10", 60},
        // {"2 * (5 + 10)", 30},
        // {"3 * 3 * 3 + 10", 37},
        // {"3 * (3 * 3) + 10", 37},
        // {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, 0);
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
        struct object *obj = test_eval(tests[i].input, 0);
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
        struct object *obj = test_eval(tests[i].input, 0);
        test_boolean_object(obj, tests[i].expected);
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
        struct object *obj = test_eval(tests[i].input, 0);
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
        struct object *obj = test_eval(tests[i].input, 0);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

void test_error_handling() {
    struct
    {
        char *input;
        char *message;
    } tests[] = {
       {
        "5 + true;",
        "type mismatch: INTEGER + BOOLEAN",
        },
        {
        "5 + true; 5;",
        "type mismatch: INTEGER + BOOLEAN",
        },
        {
        "-true",
        "unknown operator: -BOOLEAN",
        },
        {
        "true + false;",
        "unknown operator: BOOLEAN + BOOLEAN",
        },
        {
        "5; true + false; 5",
        "unknown operator: BOOLEAN + BOOLEAN",
        },
        {
        "if (10 > 1) { true + false; }",
        "unknown operator: BOOLEAN + BOOLEAN",
        },
        {
        "if (10 > 1) {               \
            if (10 > 1) {           \
                return true + false;\
            }                       \
            return 1;               \
        }", "unknown operator: BOOLEAN + BOOLEAN",
        },
        {
        "foobar",
        "identifier not found: foobar",
        },
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, 0);
        union object_value value = { .message = tests[i].message };
        test_object(obj, OBJ_ERROR, value);
    }
}

void test_let_statements() {
    struct
    {
        char *input;
        int expected;
    } tests[] = {
        {"let a = 5; a;", 5},
        {"let a = 5 * 5; a;", 25},
        {"let a = 5; let b = a; b;", 5},
        {"let a = 5; let b = a; let c = a + b + 5; c;", 15},
        {"let a = 5; let b = 5; a + b; b + a;", 10},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, 0);
        test_integer_object(obj, tests[i].expected);
    }
}

void test_function_object() {
    char *input = "fn(x) { x + 2; };";
    struct object *obj = test_eval(input, 1);

    assertf(!!obj, "expected object, got null pointers");
    assertf(obj->type == OBJ_FUNCTION, "wrong object type: expected OBJ_FUNCTION, got %s", object_type_to_str(obj->type));
    assertf(obj->function.parameters->size == 1, "wrong parameter count: expected 1, got %d", obj->function.parameters->size);

    char tmp[64];
    tmp[0] = '\0';
    identifier_list_to_str(tmp, obj->function.parameters);
    assertf(strcmp(tmp, "x") == 0, "parameter is not \"x\", got \"%s\"", tmp);

    tmp[0] = '\0';
    char *expected_body = "(x + 2)";
    block_statement_to_str(tmp, obj->function.body);
    assertf(strcmp(tmp, expected_body) == 0, "function body is not \"%s\", got \"%s\"", expected_body, tmp);
    free_program(program);
    free_object(obj);
}

void test_function_calls() {
    struct
    {
        char *input;
        int expected;
    } tests[] = {
        {"let identity = fn(x) { x; }; identity(5);", 5},
        {"let identity = fn(x) { return x; }; identity(5);", 5},
        {"let double = fn(x) { x * 2; }; double(5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
        {"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
        {"fn(x) { x; }(5)", 5},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, 0);
        test_integer_object(obj, tests[i].expected);
    }
}

void test_closing_environments() {
    char *input = "let first = 10;      \
        let second = 10;                \
        let third = 10;                 \
                                        \
        let ourFunction = fn(first) {   \
            let second = 20;            \
            first + second + third;     \
        };                              \
                                        \
        ourFunction(20) + first + second;";
    
    struct object *obj = test_eval(input, 0);
    test_integer_object(obj, 70);
}

void test_closures() {
    char *input = "             \
        let newAdder = fn(x) {  \
            fn(y) { x + y };    \
        };                      \
                                \
        let addTwo = newAdder(2);\
        addTwo(2);              \
    ";
    
    struct object *obj = test_eval(input, 0);
    test_integer_object(obj, 4);
}

void test_recursive_function() {
    char *input = "              \
        let fibonacci = fn(x) {  \
            if (x < 2) {         \
                x        \
            } else {             \
                return fibonacci(x-1) + fibonacci(x-2); \
            }                   \
        };                      \
        fibonacci(20)        \
    ";
    
    struct object *obj = test_eval(input, 0);
    test_integer_object(obj, 6765);
}

int main()
{
    test_eval_integer_expressions();
    test_eval_boolean_expressions();
    test_bang_operator();
    test_if_else_expressions();
    test_return_statements();
    test_error_handling();
    test_let_statements();
    test_function_object();
    test_function_calls();
    test_closing_environments();
    test_recursive_function();
     //test_closures();
    printf("\x1b[32mAll eval tests passed!\033[0m\n");
}