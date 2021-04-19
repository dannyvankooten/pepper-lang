#include <stdbool.h>
#include "test_helpers.h"
#include "../src/vm.h"
#include "../src/compiler.h"

union values {
    bool boolean;
    int integer;
    char *string;
    char *error;
};

static void test_object(struct object obj, enum object_type expected_type, union values expected_value) {
    assertf(obj.type == expected_type, "invalid object type: expected \"%s\", got \"%s\"", object_type_to_str(expected_type), object_type_to_str(obj.type));
    switch (expected_type) {
        case OBJ_INT:
            assertf(obj.value.integer == expected_value.integer, "invalid integer value: expected %d, got %d", expected_value.integer, obj.value.integer);
        break;
        case OBJ_BOOL:
            assertf(obj.value.boolean == expected_value.boolean, "invalid boolean value: expected %d, got %d", expected_value.boolean, obj.value.boolean);
        break;
        case OBJ_NULL: 
            // nothing to do as null objects have no further contents
        break;
        case OBJ_STRING: 
            assertf(strcmp(expected_value.string, obj.value.ptr->value) == 0, "invalid string value: expected \"%s\", got \"%s\"", expected_value.string, obj.value.ptr->value);
        break;
        case OBJ_ERROR:
            assertf(strcmp(obj.value.ptr->value, expected_value.error) == 0, "invalid error value: expected \"%s\", got \"%s\"", expected_value.error, obj.value.ptr->value);
        break;
        default: 
            assertf(false, "missing test implementation for object of type %s", object_type_to_str(obj.type));
        break;
    }

    free_object(&obj);
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
    obj = copy_object(&obj);
    free(bc);
    free_program(p);
    compiler_free(c);
    vm_free(vm);
    return obj;;
}

static void test_integer_arithmetic() {
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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_boolean_expressions() {


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
        test_object(obj, OBJ_BOOL, (union values) { .boolean = tests[t].expected });
     }
}

static void test_if_statements() {
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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}


static void test_while_statements() {
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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_nulls() {
    struct {
        const char *input;
    } tests[] = {
        {"if (1 > 2) { 10 }"},
        {"if (false) { 10 }"},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        assertf(obj.type == OBJ_NULL, "expected NULL, got %s", object_type_to_str(obj.type));
     }
}

static void test_global_let_statements() {

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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_function_calls() {

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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_functions_without_return_value() {


   const char *tests[] = {
        "let noReturn = fn() { }; noReturn();",
        "let noReturn = fn() { }; let noReturnTwo = fn() { noReturn(); }; noReturn(); noReturnTwo();"
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t]);
        test_object(obj, OBJ_NULL, (union values) {});
     }
}

static void test_first_class_functions() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let returnsOne = fn() { 1; }; let returnsOneReturner = fn() { returnsOne; }; returnsOneReturner()();", 1},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_function_calls_with_bindings() {

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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}


static void test_function_calls_with_args_and_bindings() {
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
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}


static void test_fib() {
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
    test_object(obj, OBJ_INT, (union values) { .integer = expected });    
}

static void test_recursive_functions() {

    struct {
        const char *input;
        int expected;
    } tests[] = {
        {"let countdown = fn(x) { if (x == 0) { return 0; } else { countdown(x-1); } }; countdown(3);", 0},
        {"let countdown = fn(x) { if (x < 0) { return 0; } countdown(x-1) + countdown(x-2); }; countdown(3);", 0},
    };
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union values) { .integer = tests[t].expected });
     }
}

static void test_string_expressions() {
    struct {
        const char *input;
        char *expected;
    } tests[] = {
        {"\"monkey\"", "monkey"},
        {"\"mon\" + \"key\"", "monkey"},
        {"\"mon\" + \"key\" + \"banana\"", "monkeybanana"},
    };
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_STRING, (union values) { .string = tests[t].expected });
     }
}

static void test_builtin_functions() {
    struct
    {
        const char *input;
        enum object_type type;
        union values value;
    } tests[] = {
        {"len(\"\")", OBJ_INT, {.integer = 0}},
        {"let l = len(\"a\"); puts(\"Length: \", l);", OBJ_NULL},
        {"puts(\"\", len(\"hello world\"));", OBJ_NULL},
        {"len(\"hello world\")", OBJ_INT, {.integer = 11}},
        {"len(1)", OBJ_ERROR, {.error = "argument to len() not supported: expected STRING, got INTEGER"}},
        {"len(\"one\", \"two\")", OBJ_ERROR, {.error = "wrong number of arguments: expected 1, got 2"}},
        {"type(\"one\")", OBJ_STRING, {.error = "STRING"}},
        {"type(\"one\", \"two\")", OBJ_ERROR, {.error = "wrong number of arguments: expected 1, got 2"}},
        {"puts(\"one\", \"two\")", OBJ_NULL, {}},
        {"let s = \"\"; len(s); len(s);", OBJ_INT, {.integer = 0}},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++)
    {
        struct object obj = run_vm_test(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

static void array_literals() {
    struct
    {
        const char *input;
        int nexpected;
        int expected[3];
    } tests[] = {
        {"[]", 0, {}},
        {"[1, 2, 3]", 3, {1, 2, 3}},
        {"[1 + 2, 3 * 4, 5 + 6]", 3, {3, 12, 11}},
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = run_vm_test(tests[i].input);
        assertf(obj.type == OBJ_ARRAY, "invalid obj type: expected \"%s\", got \"%s\"", object_type_to_str(OBJ_ARRAY), object_type_to_str(obj.type));

        struct object_list* arr = obj.value.ptr->value;
        assertf(tests[i].nexpected == arr->size, "invalid array size");

        for (int j=0; j < tests[i].nexpected; j++) {
            assertf(arr->values[j].type == OBJ_INT, "invalid element type");
            assertf(arr->values[j].value.integer == tests[i].expected[j], "invalid integer value: expected %d, got %d", tests[i].expected[j], arr->values[j].value.integer);
        }
        free_object(&obj);
    }
}


static void mixed_arrays() {
    struct
    {
        const char *input;
        enum object_type types[4];
        union values values[4];
    } tests[] = {
        {
            .input = "[ \"hello\", true, 0, 5 + 3]", 
            .types = { OBJ_STRING, OBJ_BOOL, OBJ_INT, OBJ_INT },
            .values = { {.string = "hello"}, { .boolean = true }, { .integer = 0 }, {.integer = 8 } }
        }
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = run_vm_test(tests[i].input);
        assertf(obj.type == OBJ_ARRAY, "invalid obj type: expected \"%s\", got \"%s\"", object_type_to_str(OBJ_ARRAY), object_type_to_str(obj.type));

        struct object_list* arr = obj.value.ptr->value;
        assertf(arr->size == 4, "invalid array size");

        for (int j=0; j < 4; j++) {
            assertf(arr->values[j].type == tests[i].types[j], "invalid element type");
            // assertf(arr->values[j].value.integer == tests[i].values[j], "invalid integer value: expected %d, got %d", tests[i].expected[j], arr->values[j].value.integer);
        }
        free_object(&obj);
    }
}

static void array_indexing() {
    struct
    {
        const char *input;
        enum object_type type;
        union values value;
    } tests[] = {
        {
            .input = "[1, 2, 3][1]", 
            .type = OBJ_INT,
            .value = { .integer = 2 }
        },
        {
            .input = "[1][0]", 
            .type = OBJ_INT,
            .value = { .integer = 1 }
        },
        {
            .input = "[1, true, \"foobar\"][2]", 
            .type = OBJ_STRING,
            .value = { .string = "foobar" }
        },
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = run_vm_test(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

static void array_indexing_out_of_bounds() {
    struct
    {
        const char *input;
        enum object_type type;
        union values value;
    } tests[] = {
        {  
            .input = "[0][1]", 
            .type = OBJ_NULL,
        },
        {   
            .input = "[0][-1]", 
            .type = OBJ_NULL,
        },
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = run_vm_test(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

static void array_pop() {
    struct
    {
        const char *input;
        enum object_type type;
        union values value;
    } tests[] = {
        {  
            .input = "array_pop([])", 
            .type = OBJ_NULL,
        },
        {   
            .input = "array_pop([1])", 
            .type = OBJ_INT,
            .value = { .integer = 1 },
        },
        {   
            .input = "array_pop([2, 1])", 
            .type = OBJ_INT,
            .value = { .integer = 1 },
        },
        {   
            .input = "let a = [1, 2, 3]; array_pop(a);", 
            .type = OBJ_INT,
            .value = { .integer = 3 },
        },
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        struct object obj = run_vm_test(tests[i].input);
        test_object(obj, tests[i].type, tests[i].value);
    }
}

static void array_push() {
    struct
    {
        const char *input;
        enum object_type type;
        union values value;
    } tests[] = {
        {  
            .input = "array_push([], 1)", 
            .type = OBJ_INT,
            .value = { .integer = 1 },
        },
        {  
            .input = "array_push([1], 1)", 
            .type = OBJ_INT,
            .value = { .integer = 2 },
        },
        {  
            .input = "let a = [1]; array_push(a, 2); a[1]", 
            .type = OBJ_INT,
            .value = { .integer = 2 },
        },
       
    };

    for (int i = 0; i < sizeof tests / sizeof tests[0]; i++) {
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
    TEST(test_string_expressions);
    TEST(test_function_calls);
    TEST(test_functions_without_return_value);
    TEST(test_first_class_functions);
    TEST(test_function_calls_with_bindings);
    TEST(test_function_calls_with_args_and_bindings);
    TEST(test_recursive_functions);
    TEST(test_fib);
    TEST(test_builtin_functions);
    TEST(array_literals);
    TEST(mixed_arrays);
    TEST(array_indexing);
    TEST(array_indexing_out_of_bounds);
    TEST(array_pop);
    TEST(array_push);
}