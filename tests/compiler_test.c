#include "test_helpers.h"
#include "compiler/compiler.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

struct compiler_test_case {
    char *input;
    int expected_constants[16];
    struct instruction *expected_instructions[16];
    unsigned int expected_instructions_size;
};

struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size) {
    struct instruction *ins = arr[0];
    for (int i = 1; i < size; i++) {
        ins->bytes = realloc(ins->bytes, (ins->size + arr[i]->size ) * sizeof(*ins->bytes));
        for (int j=0; j < arr[i]->size; j++) {
            ins->bytes[ins->size++] = arr[i]->bytes[j];
        }

        // TODO: Free instructions
    }
    return ins;
}

void test_integer_arithmetic() {
    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .expected_constants = {1, 2},
            .expected_instructions = {
                make_instruction(OP_CONST, (int[]) {0}),
                make_instruction(OP_CONST, (int[]) {1}),
            },
            .expected_instructions_size = 6,
        }
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct program *program = parse_program_str(tests[t].input);
        struct compiler *compiler = make_compiler();
        assertf(compile(compiler, program) == 0, "compiler error");
        struct bytecode *bytecode = get_bytecode(compiler);
        assertf(bytecode->instructions->size == tests[t].expected_instructions_size, "wrong instructions length: expected %d, got %d", tests[t].expected_instructions_size, bytecode->instructions->size);

        struct instruction *concatted = flatten_instructions_array(tests[t].expected_instructions, 2);
        for (int i=0; i < tests[t].expected_instructions_size; i++) {
            assertf(concatted->bytes[i] == bytecode->instructions->bytes[i], "byte mismatch");
        }
        
        // TODO: Test constants
    }
}


int main() {
    test_integer_arithmetic();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}