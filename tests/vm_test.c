#include <stdbool.h>
#include "test_helpers.h"
#include "vm/vm.h"
#include "compiler/compiler.h"

void test_integer_arithmetic() {
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
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct program *p = parse_program_str(tests[t].input);
        struct compiler *c = make_compiler();
        assertf(compile_program(c, p) == 0, "compiler error");
        struct bytecode *bc = get_bytecode(c);
        struct vm *vm = make_vm(bc);
        assertf(vm_run(vm) == 0, "vm error");
        struct object *obj = vm_stack_last_popped(vm);

        assertf(obj != NULL, "[%d] expected object, got NULL", t);
        assertf(obj->type == OBJ_INT, "[%d] invalid object type", t);
        assertf(obj->value.integer == tests[t].expected, "[%d] invalid value: expected %d, got %d", t, tests[t].expected, obj->value.integer);
    }
}

void test_boolean_expressions() {

}

int main() {
    test_integer_arithmetic();
    test_boolean_expressions();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}