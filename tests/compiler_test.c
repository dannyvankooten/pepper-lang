#include "test_helpers.h"
#include "compiler/compiler.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

struct compiler_test_case {
    char *input;
    int expected_constants[16];
    struct instruction expected_instructions[16];
};

struct instruction 
concat_instructions(struct instruction s[]) {
        struct instruction concatted = { .size = 0 };
        for (int i=0; i < s->size; i++) {
            concatted.values[concatted.size++] = s->values[i];
        }
        return concatted;
}

void test_integer_arithmetic() {
    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .expected_constants = {1, 2},
            .expected_instructions = {
                make(OP_CONST, (int[]) {0}, 1),
                make(OP_CONST, (int[]) {0}, 1),
            }
        }
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct program *program = parse_program_str(tests[t].input);
        struct compiler compiler = {};
        assertf(compile(&compiler, program) == 0, "compiler error");
        struct bytecode *bytecode = get_bytecode(&compiler);
        // struct instruction *instructions = bytecode->instructions;
        // struct instruction concatted = concat_instructions(tests[t].expected_instructions);
        // assertf(bytecode->instructions->size == concatted.size, "wrong instructions length: expected %d, got %d", concatted.size, instructions->size);
    
        // TODO: Test constants
    }
}


int main() {
    test_integer_arithmetic();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}