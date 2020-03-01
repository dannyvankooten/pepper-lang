#include "test_helpers.h"
#include "compiler/compiler.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

struct compiler_test_case {
    char *input;
    int expected_constants[16];
    size_t expected_constants_size;
    struct instruction *expected_instructions[16];
    size_t expected_instructions_size;
};

void test_integer_arithmetic() {
    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .expected_constants = {1, 2},
            .expected_constants_size = 2,
            .expected_instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_ADD),
                make_instruction(OPCODE_POP),
            },
            .expected_instructions_size = 4,
        }
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct program *program = parse_program_str(tests[t].input);
        struct compiler *compiler = make_compiler();
        assertf(compile_program(compiler, program) == 0, "compiler error");
        struct bytecode *bytecode = get_bytecode(compiler);
        struct instruction *concatted = flatten_instructions_array(tests[t].expected_instructions, tests[t].expected_instructions_size);

        assertf(bytecode->instructions->size == concatted->size, "wrong instructions length: expected \"%s\", got \"%s\"", instruction_to_str(concatted), instruction_to_str(bytecode->instructions));
        for (int i=0; i < concatted->size; i++) {
            assertf(concatted->bytes[i] == bytecode->instructions->bytes[i], "byte mismatch");
        }

        assertf(bytecode->constants->size == tests[t].expected_constants_size, "wrong constants size: expected %d, got %d", tests[t].expected_constants_size, bytecode->constants->size);
        for (int i=0; i < tests[t].expected_constants_size; i++) {
            assertf(bytecode->constants->values[i]->integer == tests[t].expected_constants[i], "invalid constant: expected %d, got %d", tests[t].expected_constants[i], bytecode->constants->values[i]->integer);
        }

        free_compiler(compiler);
    }
}

int main() {
    test_integer_arithmetic();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}