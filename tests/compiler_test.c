#include "test_helpers.h"
#include "compiler/compiler.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

struct compiler_test_case {
    char *input;
    struct {
        union object_value value;
        enum object_type type;
    } constants[16];
    size_t constants_size;
    struct instruction *instructions[16];
    size_t instructions_size;
};

void test_object(struct object *obj, enum object_type type, union object_value value) {
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

void run_compiler_tests(const char *test_name, struct compiler_test_case tests[], size_t n) {
    strcpy(current_test, test_name);

    for (int t=0; t < n; t++) {
        struct program *program = parse_program_str(tests[t].input);
        struct compiler *compiler = make_compiler();
        int err = compile_program(compiler, program);
        assertf(err == 0, "compiler error: %s", compiler_error_str(err));
        struct bytecode *bytecode = get_bytecode(compiler);
        struct instruction *concatted = flatten_instructions_array(tests[t].instructions, tests[t].instructions_size);

        char *concatted_str = instruction_to_str(concatted);
        char *bytecode_str = instruction_to_str(bytecode->instructions);
        assertf(bytecode->instructions->size == concatted->size, "wrong instructions length: expected \"%s\", got \"%s\"", concatted_str, bytecode_str);
        for (int i=0; i < concatted->size; i++) {
            assertf(concatted->bytes[i] == bytecode->instructions->bytes[i], "byte mismatch");
        }

        assertf(bytecode->constants->size == tests[t].constants_size, "wrong constants size: expected %d, got %d", tests[t].constants_size, bytecode->constants->size);
        for (int i=0; i < tests[t].constants_size; i++) {
            test_object(bytecode->constants->values[i], tests[t].constants[i].type, tests[t].constants[i].value);
        }

        free(concatted_str);
        free(bytecode_str);
        free(bytecode);
        free_program(program);
        free_compiler(compiler);
    }
}

#define OBJECT_INT(v) { .type = OBJ_INT, .value = { .integer = v } }

void test_integer_arithmetic() {
    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .constants = {OBJECT_INT(1), OBJECT_INT(2)},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_ADD),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
        {
            .input = "1 - 2",
            .constants = {OBJECT_INT(1), OBJECT_INT(2)},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_SUBTRACT),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
        {
            .input = "1 * 2",
            .constants = {OBJECT_INT(1), OBJECT_INT(2)},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_MULTIPLY),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
        {
            .input = "2 / 1",
            .constants = {OBJECT_INT(2), OBJECT_INT(1)},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_DIVIDE),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
    };

    run_compiler_tests(__FUNCTION__, tests, ARRAY_SIZE(tests));
}

void test_boolean_expressions() {
    struct compiler_test_case tests[] = {
        {
            .input = "true",
            .constants = {},
            .constants_size = 0,
            .instructions = {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 2,
        },
        {
            .input = "false",
            .constants = {},
            .constants_size = 0,
            .instructions = {
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 2,
        }
    };

    run_compiler_tests(__FUNCTION__, tests, ARRAY_SIZE(tests));
}

int main() {
    test_integer_arithmetic();
   // test_boolean_expressions();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}