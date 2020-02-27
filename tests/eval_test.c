#include <stdio.h>
#include <string.h> 
#include <stdbool.h>

#include "eval/eval.h"
#include "test_helpers.h"

// declared here so we can free it from other tests
struct program *program;

void test_environment() {
    struct environment *env = make_environment(26);

    // set
    struct object o1 = {.integer = 1 };
    struct object o2 = {.integer = 2};

    environment_set(env, "foo", &o1);
    environment_set(env, "bar", &o2);
    
    // get
    struct object *r1 = environment_get(env, "foo");
    assertf(r1->integer == o1.integer, "expected %d, got %d", o1.integer, r1->integer);
    struct object *r2 = environment_get(env, "bar");
    assertf(r2->integer == o2.integer, "expected %d, got %d", o2.integer, r2->integer);

    // not existing
    assertf(environment_get(env, "unexisting") == NULL, "expected NULL, got something");

    // free env
    free(env->table);
    free(env);
}

struct object *test_eval(char *input, bool keep_prog)
{
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    program = parse_program(&parser);

    if (parser.errors > 0) {
        printf("parsing errors: \n");
        for (int i=0; i < parser.errors; i++) {
            printf("  %s\n", parser.error_messages[i]);
        }
        exit(1);
    }

    struct environment *env = make_environment(26);
    struct object *obj = eval_program(program, env);

    // Free'ing the program clears the identifier values, which we need in test_function_object
    if (!keep_prog) {
        free_program(program);
    }

    free_environment(env);
    free_env_pool();
    free_object_list_pool();
    free_object_pool();
    return obj;
}

void test_integer_object(struct object *obj, int expected)
{
    assertf(!!obj, "expected integer object, got null pointer");
    assertf(obj->type == OBJ_INT, "wrong object type: expected %s, got %s %s", object_type_to_str(OBJ_INT), object_type_to_str(obj->type), obj->error);
    assertf(obj->integer == expected, "wrong integer value: expected %d, got %d", expected, obj->integer);
}

void test_boolean_object(struct object *obj, char expected)
{
    assertf(!!obj, "expected boolean object, got null pointer");
    assertf(obj->type == OBJ_BOOL, "wrong object type: expected %s, got %s %s", object_type_to_str(OBJ_BOOL), object_type_to_str(obj->type), obj->error);
    assertf(obj->boolean == expected, "wrong boolean value: expected %d, got %d", expected, obj->boolean);
}

void test_string_object(struct object *obj, char *expected)
{
    assertf(!!obj, "expected string object, got null pointer");
    assertf(obj->type == OBJ_STRING, "wrong object type: expected %s, got %s %s", object_type_to_str(OBJ_STRING), object_type_to_str(obj->type), obj->error);
    assertf(strcmp(obj->string, expected) == 0, "wrong string value: expected %s, got %s", expected, obj->string);
}

union object_value {
    int integer;
    char null;
    bool boolean;
    char *message;
    char *string;
};

void test_error_object(struct object *obj, char *expected) {
    assertf(!!obj, "expected error object, got null pointer");
    assertf(obj->type == OBJ_ERROR, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_ERROR), object_type_to_str(obj->type));
    assertf(strcmp(obj->error, expected) == 0, "invalid error message: expected %s, got %s", expected, obj->error);
}

void test_object(struct object *obj, enum object_type type, union object_value value)
{
    switch (type)
    {
    case OBJ_BOOL:
        test_boolean_object(obj, value.boolean);
        break;
    case OBJ_INT:
        test_integer_object(obj, value.integer);
        break;
    case OBJ_ERROR:
        test_error_object(obj, value.message);
        break;    
    case OBJ_STRING:
        test_string_object(obj, value.string);
        break;
    case OBJ_NULL: 
        assertf(obj->type == OBJ_NULL, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_NULL), object_type_to_str(obj->type));
        break;
    default:
        printf("No test function for object of type %s\n", object_type_to_str(type));
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
        struct object *obj = test_eval(tests[i].input, false);
        test_integer_object(obj, tests[i].expected);
        free_object(obj);
    }
}


void test_eval_string_expression()
{
    struct object *obj = test_eval("\"Hello world!\"", false);
    test_string_object(obj, "Hello world!");
    free_object(obj);
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
        struct object *obj = test_eval(tests[i].input, false);
        test_boolean_object(obj, tests[i].expected);
        free_object(obj);
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
        struct object *obj = test_eval(tests[i].input, false);
        test_boolean_object(obj, tests[i].expected);
        free_object(obj);
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
        struct object *obj = test_eval(tests[i].input, false);
        test_object(obj, tests[i].type, tests[i].value);
        free_object(obj);
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
        struct object *obj = test_eval(tests[i].input, false);
        test_object(obj, tests[i].type, tests[i].value);
        free_object(obj);
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
        {
            "\"Hello\" - \"world\"",
            "unknown operator: STRING - STRING",
        }
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, false);
        union object_value value = { .message = tests[i].message };
        test_object(obj, OBJ_ERROR, value);
        free_object(obj);
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
        struct object *obj = test_eval(tests[i].input, false);
        test_integer_object(obj, tests[i].expected);
        free_object(obj);
    }
}

void test_function_object() {
    char *input = "fn(x) { x + 2; };";
    struct object *obj = test_eval(input, true);

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
        struct object *obj = test_eval(tests[i].input, false);
        test_integer_object(obj, tests[i].expected);
        free_object(obj);
    }
}

void test_closing_environments() {
    char *input = "let a = 10;      \
        let b = 10;                \
        let c = 10;                 \
        let ourFunction = fn(a) {   \
            let b = 20;            \
            a + b + c;     \
        };                              \
        let c = c * 2;   \
                                        \
        ourFunction(20) + a + b";
    
    struct object *obj = test_eval(input, false);
    test_integer_object(obj, 80);
    free_object(obj);
}

void test_closures() {
    char *input = "             \
        let newAdder = fn(x) {  \
            fn(y) { x + y };    \
        };                      \
        let addTwo = newAdder(2);\
        addTwo(2);              \
    ";
    
    struct object *obj = test_eval(input, false);
    test_integer_object(obj, 4);
    free_object(obj);
}


void test_invalid_function() {
    char *input = ""
        "let my_function = fn(a, b) { 100 };"
        "my_function(20)";
    
    struct object *obj = test_eval(input, false);
    test_error_object(obj, "invalid function call: expected 2 arguments, got 1");
    free_object(obj);
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
    
    struct object *obj = test_eval(input, false);
    test_integer_object(obj, 6765);
    free_object(obj);
}

void test_invalid_function_call() {
    char *input = "              \
        let my_function = fn(a, b) { 100 };      \
        my_function(20)";

    struct object *obj = test_eval(input, false);
    test_error_object(obj, "invalid function call: expected 2 arguments, got 1");
    free_object(obj);
}

void test_shadow_declaration() {
    char *input = "let count = 0;"
        "let shadow = fn() { let count = 1; return count; }; "
        "return shadow();";
    struct object *obj = test_eval(input, false);
    test_integer_object(obj, 1);
    free_object(obj);
}

void test_actual_code() {
    char *input = " \
        let a = 100;\
        let b = 200;\
        let add = fn (a, b) {\
           let tmp = a;\
           let a = b;\
           let b = tmp;\
           return a + b;\
        };\
        let multiply = fn(a, b) { return b * a; };\
        if (a) {\
            if (add(100, a) == 200) {\
                if (multiply(a, b) == 20000) {\
                    return b;\
                }\
            }\
        }\
        \
        return -1;\
    ";

    struct object *obj = test_eval(input, false);
    test_integer_object(obj, 200);
    free_object(obj);
}

void test_string_concatenation() {
    char *input = "\"Hello\" + \" \" + \"world\"";
    struct object *obj = test_eval(input, false);
    test_string_object(obj, "Hello world");
    free_object(obj);
}

void test_builtin_functions() {
    struct
    {
        char *input;
        enum object_type type;
        union object_value value;
    } tests[] = {
        {"len(\"\")", OBJ_INT, {.integer = 0}},
        {"len(\"four\")", OBJ_INT, {.integer = 4}},
        {"len(\"hello world\")", OBJ_INT, {.integer = 11}},
        {"len(1)", OBJ_ERROR, {.message = "argument to len() not supported: expected STRING, got INTEGER"}},
        {"len(\"one\", \"two\")", OBJ_ERROR, {.message = "wrong number of arguments: expected 1, got 2"}},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object *obj = test_eval(tests[i].input, false);
        test_object(obj, tests[i].type, tests[i].value);
        free_object(obj);
    }
}

void test_array_literals() {
    char *input = "[1, 2 * 2, 3 + 3]";
    struct object *obj = test_eval(input, false);
    assertf(!!obj, "expected object, got null pointer");
    assertf(obj->type == OBJ_ARRAY, "wrong object type: expected %s, got %s", object_type_to_str(OBJ_ARRAY), object_type_to_str(obj->type));
    assertf(obj->array->size == 3, "wrong array size: expected %d, got %d", 3, obj->array->size);
    test_integer_object(obj->array->values[0], 1);
    test_integer_object(obj->array->values[1], 4);
    test_integer_object(obj->array->values[2], 6);
    free_object(obj);
}

void test_array_index_expressions() {
    struct {
        char *input;
        int expected;
    } tests[] = {
        { "[1, 2, 3][0]", 1},
        { "[1, 2, 3][1]", 2},
        { "[1, 2, 3][2]", 3},
        { "let i = 0; [1][i];", 1},
        { "[1, 2, 3][1 + 1];", 3},
        { "let myArray = [1, 2, 3]; myArray[2];", 3},
        { "let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];", 6},
        { "let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i]", 2}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object *obj = test_eval(tests[i].input, false);
        test_integer_object(obj, tests[i].expected);
        free_object(obj);
    }

    // TODO: Test out of bounds indexing
}


void test_while_expressions() {
    struct {
        char *input;
        int expected;
    } tests[] = {
        {"let a = 0; while (1 > 3) { let a = a + 1; }; a;", 0},
        {"let a = 0; while (a < 3) { let a = a + 1; }; a;", 3},
        {"let a = 1; while (a < 3) { let a = a + 1; a; };", 3},
        {"let a = 1; while (a < 3) { let a = a + 1; };", 3}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object *obj = test_eval(tests[i].input, false);
        test_integer_object(obj, tests[i].expected);
        free_object(obj);
    }

    // TODO: Test out of bounds indexing
}

int main()
{
    test_environment();
    test_eval_integer_expressions();
    test_eval_boolean_expressions();
    test_eval_string_expression();
    test_bang_operator();
    test_if_else_expressions();
    test_return_statements();
    test_error_handling();
    test_let_statements();
    test_function_object();
    test_function_calls();
    test_closing_environments();
    test_recursive_function();
    test_invalid_function_call();
    test_shadow_declaration();
    test_actual_code();
    test_string_concatenation();
    test_builtin_functions();
    test_array_literals();
    test_array_index_expressions();
    test_while_expressions();

    // TODO: Fix closures 
    // This is tricky because we can't just clear out the outer environment, 
    // we need to extend it's lifetime somehow
    // test_closures();

    printf("\x1b[32mAll eval tests passed!\033[0m\n");
}