#include <stdbool.h>
#include "test_helpers.h"
#include "../src/vm.h"
#include "../src/compiler.h"

void test_object(struct object obj, enum object_type type, union object_value value) {
    assertf(obj.type == type, "invalid object type: expected \"%s\", got \"%s\"", object_type_to_str(type), object_type_to_str(obj.type));
    switch (type) {
        case OBJ_INT:
            assertf(obj.value.integer == value.integer, "invalid integer value: expected %d, got %d", value.integer, obj.value.integer);
        break;
        case OBJ_BOOL:
            assertf(obj.value.boolean == value.boolean, "invalid boolean value: expected %d, got %d", value.boolean, obj.value.boolean);
        break;
        case OBJ_NULL: 
            // nothing to do as null objects have no further contents
        break;
        case OBJ_STRING: 
            assertf(strcmp(value.string, obj.value.string) == 0, "invalid string value: expected \"%s\", got \"%s\"", value.string, obj.value.string);
            free(obj.value.string);
        break;
        case OBJ_ERROR:
            assertf(strcmp(obj.value.error, value.error) == 0, "invalid error value: expected \"%s\", got \"%s\"", value.error, obj.value.error);
            free(obj.value.error);
        break;
        default: 
            assertf(false, "missing test implementation for object of type %s", object_type_to_str(obj.type));
        break;
    }
}

struct object 
run_vm_test(const char *program_str) {
    struct program *p = parse_program_str(program_str);
    struct compiler *c = compiler_new();
    int err = compile_program(c, p);
    assertf(err == 0, "compiler error: %s", compiler_error_str(err));
    struct bytecode *bc = get_bytecode(c);
    struct vm *vm = vm_new(bc);
    err = vm_run(vm);
    assertf(err == 0, "vm error: %d", err);
    struct object obj = vm_stack_last_popped(vm);

    // TODO: We should only duplicate string if comes from a constant, since these are freed elsewhere
    // Even better would be to take ownership of every string entering the VM
    if (obj.type == OBJ_STRING) {
        obj.value.string = strdup(obj.value.string);
    }
    
    free(bc);
    free_program(p);
    compiler_free(c);
    vm_free(vm);
    return obj;;
}

void test_integer_arithmetic() {


    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"1", 1},
        {"2", 2},
        {"1 + 2", 3}, 
        {"1 - 2", -1},
        {"1 * 2", 2},
        {"4 / 2", 2},
        {"50 / 2 * 2 + 10 - 5", 55}, 
        {"5 + 5 + 5 + 5 - 10", 10}, 
        {"2 * 2 * 2 * 2 * 2", 32}, 
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"5 * (2 + 10)", 60},
        {"-5", -5},
        {"-10", -10},
        {"-50 + 100 + -50", 0},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_boolean_expressions() {


    struct {
        const char *input;
        bool expected;
    } tests[] = {
        {"true", true},
        {"false", false},
        {"1 < 2", true},
        {"1 > 2", false},
        {"1 < 1", false},
        {"1 > 1", false},
        {"1 == 1", true},
        {"1 != 1", false},
        {"1 == 2", false},
        {"1 != 2", true},
        {"true == true", true},
        {"false == false", true},
        {"true == false", false},
        {"true != false", true},
        {"false != true", true},
        {"(1 < 2) == true", true},
        {"(1 < 2) == false", false},
        {"(1 > 2) == true", false},
        {"(1 > 2) == false", true},
        {"!true", false},
        {"!false", true},
        {"!5", false},
        {"!!true", true},
        {"!!false", false},
        {"!!5", true},
        {"!(if (false) { 5; })", true},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_BOOL, (union object_value) { .boolean = tests[t].expected });
     }
}

void test_if_statements() {
    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"if (true) { 10 }", 10},
        {"if (true) { 10 } else { 20 }", 10},
        {"if (false) { 10 } else { 20 } ", 20},
        {"if (1) { 10 }", 10},
        {"if (1 < 2) { 10 }", 10},
        {"if (1 < 2) { 10 } else { 20 }", 10},
        {"if (1 > 2) { 10 } else { 20 }", 20},
        {"if ((if (false) { 10 })) { 10 } else { 20 }", 20}
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}


void test_while_statements() {
    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"while (false) { 10 }; 5", 5},
        {"let a = 2; while (1 > 3) { let a = a + 1; }; a;", 2},
        {"let a = 0; while (a < 3) { let a = a + 1; }; a;", 3},
        {"let a = 1; while (a < 3) { let a = a + 1; a; };", 3},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_nulls() {

    struct {
        const char *input;
        struct object obj;
    } tests[] = {
        {"if (1 > 2) { 10 }", obj_null},
        {"if (false) { 10 }", obj_null},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        assertf(obj.type == OBJ_NULL, "expected NULL, got %s", object_type_to_str(obj.type));
     }
}

void test_global_let_statements() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let one = 1; one", 1},
        {"let one = 1; let two = 2; one + two", 3},
        {"let one = 1; let two = one + one; one + two", 3},
        {"let one = 1; let one = one + 1;", 2},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_function_calls() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let fivePlusTen = fn() { 5 + 10; }; fivePlusTen();", 15},
        {"let one = fn() { 1; }; let two = fn() { 2; } one() + two();", 3},
        {"let a = fn() { 1 }; let b = fn() { a() + 1 }; let c = fn() { b() + 1 }; c();", 3},
        {"let earlyExit = fn() { return 99; 100; }; earlyExit();", 99},
        {"let earlyExit = fn() { return 99; return 100; }; earlyExit();", 99},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_functions_without_return_value() {


   const char *tests[] = {
        "let noReturn = fn() { }; noReturn();",
        "let noReturn = fn() { }; let noReturnTwo = fn() { noReturn(); }; noReturn(); noReturnTwo();"
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t]);
        test_object(obj, OBJ_NULL, (union object_value) {});
     }
}

void test_first_class_functions() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let returnsOne = fn() { 1; }; let returnsOneReturner = fn() { returnsOne; }; returnsOneReturner()();", 1},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_function_calls_with_bindings() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let one = fn() { let one = 1; one }; one();", 1},
        {"let oneAndTwo = fn() { let one = 1; let two = 2; one + two; }; oneAndTwo();", 3},
        {"let oneAndTwo = fn() { let one = 1; let two = 2; one + two; }; let threeAndFour = fn() { let three = 3; let four = 4; three + four; }; oneAndTwo() + threeAndFour();", 10},
        {"let firstFoobar = fn() { let foobar = 50; foobar; }; let secondFoobar = fn() { let foobar = 100; foobar; }; firstFoobar() + secondFoobar();", 150},
        {"let globalSeed = 50; let minusOne = fn() { let num = 1; globalSeed - num; } let minusTwo = fn() { let num = 2; globalSeed - num; } minusOne() + minusTwo();", 97},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}


void test_function_calls_with_args_and_bindings() {
    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let identity = fn(a) { a; }; identity(4);", 4},
        {"let sum = fn(a, b) { a + b; }; sum(1, 2);", 3},
        {"let sum = fn(a, b) { let c = a + b; c; }; sum(1, 2);", 3},
        {"let sum = fn(a, b) { let c = a + b; c; }; sum(1, 2) + sum(3, 4);", 10},
        {"let sum = fn(a, b) { let c = a + b; c; }; let outer = fn() { sum(1, 2) + sum(3, 4); }; outer();", 10},
        {"let sum = fn(a, b) { let c = a + b; c; }; let outer = fn() { sum(1, 2) + sum(3, 4); }; outer();", 10},
        {"let globalNum = 10;\
          let sum = fn(a, b) {\
               let c = a + b;\
               c + globalNum;\
          };\
            let outer = fn() {\
                sum(1, 2) + sum(3, 4) + globalNum;\
            };\
            outer() + globalNum;", 50},
    };
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}


void test_fib() {
    const char *input = "              \
        let fibonacci = fn(x) {  \
            if (x < 2) {         \
                return x;        \
            }                    \
            return fibonacci(x-1) + fibonacci(x-2); \
        };                      \
        fibonacci(6)";
    int expected = 8;
    struct object obj = run_vm_test(input);
    test_object(obj, OBJ_INT, (union object_value) { .integer = expected });    
}

void test_recursive_functions() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let countdown = fn(x) { if (x == 0) { return 0; } else { countdown(x-1); } }; countdown(3);", 0},
        {"let countdown = fn(x) { if (x < 0) { return 0; } countdown(x-1) + countdown(x-2); }; countdown(3);", 0},
    };
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_string_expressions() {

    struct {
        const char *input;
        char *expected;
    } tests[] = {
        {"\"monkey\"", "monkey"},

        // TODO: Fix memory leak in below operations
        // {"\"mon\" + \"key\"", "monkey"},
        // {"\"mon\" + \"key\" + \"banana\"", "monkeybanana"},
    };
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_STRING, (union object_value) { .string = tests[t].expected });
     }
}

void test_builtin_functions() {

    struct
    {
        const char *input;
        enum object_type type;
        union object_value value;
    } tests[] = {
        {"len(\"\")", OBJ_INT, {.integer = 0}},
        {"len(\"hello world\")", OBJ_INT, {.integer = 11}},
        {"len(1)", OBJ_ERROR, {.error = "argument to len() not supported: expected STRING, got INTEGER"}},
        {"len(\"one\", \"two\")", OBJ_ERROR, {.error = "wrong number of arguments: expected 1, got 2"}},
        // {"type(\"one\")", OBJ_STRING, {.string = "STRING"}},
        {"type(\"one\", \"two\")", OBJ_ERROR, {.error = "wrong number of arguments: expected 1, got 2"}},
        //  {"puts(\"one\", \"two\")", OBJ_NULL, {}},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object obj = run_vm_test(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

int main(int argc, const char *argv[]) {
    TEST(test_integer_arithmetic);
    TEST(test_boolean_expressions);
    TEST(test_if_statements);
    TEST(test_nulls);
    TEST(test_global_let_statements);
    TEST(test_while_statements);
    TEST(test_function_calls);
    TEST(test_functions_without_return_value);
    TEST(test_first_class_functions);
    TEST(test_function_calls_with_bindings);
    TEST(test_function_calls_with_args_and_bindings);
    TEST(test_recursive_functions);
    TEST(test_fib);
    TEST(test_string_expressions);
    TEST(test_builtin_functions);

    free_object_pool();
    free_object_list_pool();
}