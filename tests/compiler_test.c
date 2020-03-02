#include "test_helpers.h"
#include "compiler/compiler.h"

struct compiler_test_case {
    char *input;
    int constants[16];
    size_t constants_size;
    struct instruction *instructions[16];
    size_t instructions_size;
};

void test_object(struct object *obj, enum object_type type, union object_value value) {
    assertf(obj != NULL, "expected object, got null");
    assertf(obj->type == type, "invalid object type: expected %s, got %s", object_type_to_str(type), object_type_to_str(obj->type));
    switch (type) {
        case OBJ_INT:
            assertf(obj->value.integer == value.integer, "invalid integer value: expected %d, got %d", value.integer, obj->value.integer);
        break;
        case OBJ_BOOL:
            assertf(obj->value.boolean == value.boolean, "invalid boolean value: expected %d, got %d", value.boolean, obj->value.boolean);
        break;
    }
}

void run_compiler_tests(struct compiler_test_case tests[], size_t n) {
   
    for (int t=0; t < n; t++) {
        struct program *program = parse_program_str(tests[t].input);
        struct compiler *compiler = make_compiler();
        int err = compile_program(compiler, program);
        assertf(err == 0, "compiler error: %s", compiler_error_str(err));
        struct bytecode *bytecode = get_bytecode(compiler);
        struct instruction *concatted = flatten_instructions_array(tests[t].instructions, tests[t].instructions_size);

        char *concatted_str = instruction_to_str(concatted);
        char *bytecode_str = instruction_to_str(bytecode->instructions);
        assertf(bytecode->instructions->size == concatted->size, "wrong instructions length: \nexpected\n\"%s\"\ngot\n\"%s\"", concatted_str, bytecode_str);
        
        for (int i=0; i < concatted->size; i++) {
            assertf(concatted->bytes[i] == bytecode->instructions->bytes[i], "byte mismatch at pos %d: expected '%d', got '%d'\nexpected: %s\ngot: %s\n", i, concatted->bytes[i], bytecode->instructions->bytes[i], concatted_str, bytecode_str);
        }

        assertf(bytecode->constants->size == tests[t].constants_size, "wrong constants size: expected %d, got %d", tests[t].constants_size, bytecode->constants->size);
        for (int i=0; i < tests[t].constants_size; i++) {
            test_object(bytecode->constants->values[i], OBJ_INT, (union object_value) { .integer = tests[t].constants[i] });
        }

        free(concatted_str);
        free(bytecode_str);
        free(bytecode);
        free_program(program);
        free_compiler(compiler);
    }

    TESTNAME("");
}

void test_integer_arithmetic() {
    TESTNAME(__FUNCTION__);

    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .constants = {1, 2},
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
            .constants = {1, 2},
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
            .constants = {1, 2},
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
            .constants = {2, 1},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_DIVIDE),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
        {
            .input = "2 / 1",
            .constants = {2, 1},
            .constants_size = 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_DIVIDE),
                make_instruction(OPCODE_POP),
            },
            .instructions_size = 4,
        },
        {
            .input = "-1",
            .constants = {1}, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_MINUS),
                make_instruction(OPCODE_POP),
            }, 3,
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void test_boolean_expressions() {
    TESTNAME(__FUNCTION__);

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
            "false",
            {}, 0,
            {
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_POP),
            },
            2,
        },
        {
            "1 > 2", 
            {1, 2}, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_GREATER_THAN),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 < 2", 
            {1, 2}, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_LESS_THAN),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 == 2", 
            {1, 2}, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_EQUAL),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 != 2", 
            {1, 2}, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_NOT_EQUAL),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "true == false", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_EQUAL),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "true != false", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_NOT_EQUAL),
                make_instruction(OPCODE_POP),
            }, 4
        },
         {
            "!true", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_BANG),
                make_instruction(OPCODE_POP),
            }, 3
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void test_conditionals() {
    TESTNAME(__FUNCTION__);

    struct compiler_test_case tests[] = {
        {
            .input = "if (true) { 10; } 3333;",
            .constants = {10, 3333 }, 2,
            .instructions = {
                make_instruction(OPCODE_TRUE),              // 0000
                make_instruction(OPCODE_JUMP_NOT_TRUE, 7),  // 0001
                make_instruction(OPCODE_CONST, 0),          // 0004
                make_instruction(OPCODE_POP),               // 0007
                make_instruction(OPCODE_CONST, 1),          // 0008
                make_instruction(OPCODE_POP)                // 0011
            }, 6
        },
        {
            .input = "if (true) { 10; } else { 20; }; 3333;",
            .constants = {10, 20, 3333 }, 3,
            .instructions = {
                make_instruction(OPCODE_TRUE),              // 0000
                make_instruction(OPCODE_JUMP_NOT_TRUE, 10), // 0001
                make_instruction(OPCODE_CONST, 0),          // 0004
                make_instruction(OPCODE_JUMP, 13),          // 0007
                make_instruction(OPCODE_CONST, 1),          // 0010
                make_instruction(OPCODE_POP),               // 0013
                make_instruction(OPCODE_CONST, 2),          // 0014
                make_instruction(OPCODE_POP),               // 0017
            }, 8
        },
    };
    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

int main() {
    test_integer_arithmetic();
    test_boolean_expressions();
    test_conditionals();
    printf("\x1b[32mAll tests passed!\033[0m\n");
}