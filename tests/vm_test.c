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
        {"1 + 2", 2}, // FIXME
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct program *p = parse_program_str(tests[t].input);
        struct compiler *c = make_compiler();
        assertf(compile_program(c, p) == 0, "compiler error");
        struct bytecode *bc = get_bytecode(c);
        struct vm *vm = make_vm(bc);
        assertf(vm_run(vm) == 0, "vm error");
        struct object *obj = vm_stack_top(vm);

        assertf(obj != NULL, "expected object, got NULL");
        assertf(obj->type == OBJ_INT, "invalid object type");
        assertf(obj->integer == tests[t].expected, "invalid value: expected %d, got %d", tests[t].expected, obj->integer);
    }
}

int main() {
    test_integer_arithmetic();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}