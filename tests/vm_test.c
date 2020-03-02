#include <stdbool.h>
#include "test_helpers.h"
#include "vm/vm.h"
#include "compiler/compiler.h"

void test_object(struct object *obj, enum object_type type, union object_value value) {
    assertf(obj != NULL, "expected object, got null");
    assertf(obj->type == type, "invalid object type");
    switch (type) {
        case OBJ_INT:
            assertf(obj->value.integer == value.integer, "invalid integer value: expected %d, got %d", value.integer, obj->value.integer);
        break;
        case OBJ_BOOL:
            assertf(obj->value.boolean == value.boolean, "invalid boolean value: expected %d, got %d", value.boolean, obj->value.boolean);
        break;

    }
}

struct object *run_vm_test(char *program_str) {
    struct program *p = parse_program_str(program_str);
    struct compiler *c = make_compiler();
    int err = compile_program(c, p);
    assertf(err == 0, "compiler error: %s", compiler_error_str(err));
    struct bytecode *bc = get_bytecode(c);
    struct vm *vm = make_vm(bc);
    err = vm_run(vm);
    assertf(err == 0, "vm error: %d", err);
    struct object *obj = vm_stack_last_popped(vm);

    // TODO: Free vm
    free(bc);
    free_compiler(c);
    free_program(p);
    return obj;
}

void test_integer_arithmetic() {
    TESTNAME(__FUNCTION__);

    struct {
        char *input;
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
        struct object *obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

void test_boolean_expressions() {
    TESTNAME(__FUNCTION__);

    struct {
        char *input;
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
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object *obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_BOOL, (union object_value) { .boolean = tests[t].expected });
     }
}

void test_conditionals() {
    TESTNAME(__FUNCTION__);

    struct {
        char *input;
        int expected;
    } tests[] = {
        {"if (true) { 10 }", 10},
        {"if (true) { 10 } else { 20 }", 10},
        {"if (false) { 10 } else { 20 } ", 20},
        {"if (1) { 10 }", 10},
        {"if (1 < 2) { 10 }", 10},
        {"if (1 < 2) { 10 } else { 20 }", 10},
        {"if (1 > 2) { 10 } else { 20 }", 20},
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct object *obj = run_vm_test(tests[t].input);
        test_object(obj, OBJ_INT, (union object_value) { .integer = tests[t].expected });
     }
}

int main() {
    test_integer_arithmetic();
    test_boolean_expressions();
    test_conditionals();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}