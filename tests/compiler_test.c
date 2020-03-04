#include "test_helpers.h"
#include "compiler.h"

struct compiler_test_case {
    char *input;
    struct object *constants[16];
    size_t constants_size;
    struct instruction *instructions[16];
    size_t instructions_size;
};

void test_object(struct object *expected, struct object *actual) {
    assertf(actual != NULL, "expected object, got null");
    assertf(actual->type == expected->type, "invalid object type: expected %s, got %s", object_type_to_str(expected->type), object_type_to_str(actual->type));
    
    switch (expected->type) {
        case OBJ_INT:
            assertf(actual->value.integer == expected->value.integer, "invalid integer value: expected %d, got %d", expected->value.integer, actual->value.integer);
        break;
        case OBJ_BOOL:
            assertf(actual->value.boolean == expected->value.boolean, "invalid boolean value: expected %d, got %d", expected->value.boolean, actual->value.boolean);
        break;
        case OBJ_COMPILED_FUNCTION: {
            char *expected_str = instruction_to_str(expected->value.compiled_function);
            char *actual_str = instruction_to_str(actual->value.compiled_function);
            assertf(expected->value.compiled_function->size == actual->value.compiled_function->size, "wrong instructions length: \nexpected\n\"%s\"\ngot\n\"%s\"", expected_str, actual_str);
            for (int i=0; i < expected->value.compiled_function->size; i++) {
                assertf(expected->value.compiled_function->bytes[i] == actual->value.compiled_function->bytes[i], "byte mismatch at pos %d: expected '%d', got '%d'\nexpected: %s\ngot: %s\n", i, expected->value.compiled_function->bytes[i], actual->value.compiled_function->bytes[i], expected_str, actual_str);
            }
            free(expected_str);
            free(actual_str);
        }
        break;
        default: 
            assertf(false, "missing test implementation for object of type %s", object_type_to_str(actual->type));
        break;
    }
}

void run_compiler_test(struct compiler_test_case t) {
    struct program *program = parse_program_str(t.input);
    struct compiler *compiler = compiler_new();
    int err = compile_program(compiler, program);
    assertf(err == 0, "compiler error: %s", compiler_error_str(err));
    struct bytecode *bytecode = get_bytecode(compiler);
    struct instruction *concatted = flatten_instructions_array(t.instructions, t.instructions_size);

    char *concatted_str = instruction_to_str(concatted);
    char *bytecode_str = instruction_to_str(bytecode->instructions);
    assertf(bytecode->instructions->size == concatted->size, "wrong instructions length: \nexpected\n\"%s\"\ngot\n\"%s\"", concatted_str, bytecode_str);
    
    for (int i=0; i < concatted->size; i++) {
        assertf(concatted->bytes[i] == bytecode->instructions->bytes[i], "byte mismatch at pos %d: expected '%d', got '%d'\nexpected: %s\ngot: %s\n", i, concatted->bytes[i], bytecode->instructions->bytes[i], concatted_str, bytecode_str);
    }

    assertf(bytecode->constants->size == t.constants_size, "wrong constants size: expected %d, got %d", t.constants_size, bytecode->constants->size);
    for (int i=0; i < t.constants_size; i++) {
        test_object(t.constants[i], bytecode->constants->values[i]);
    }

    // TODO: Free objects

    free(concatted_str);
    free(bytecode_str);
    free(bytecode);
    free_program(program);
    compiler_free(compiler);
}

void run_compiler_tests(struct compiler_test_case tests[], size_t n) {
    for (int t=0; t < n; t++) {
       run_compiler_test(tests[t]);
    }
}

void test_integer_arithmetic() {
    TESTNAME(__FUNCTION__);

    struct compiler_test_case tests[] = {
        {
            .input = "1 + 2",
            .constants = {
                make_integer_object(1), 
                make_integer_object(2),
            },
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
            .constants = {
                make_integer_object(1), 
                make_integer_object(2),
            },
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
            .constants = {
                make_integer_object(1), 
                make_integer_object(2),
            },
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
            .constants = {
                make_integer_object(2), 
                make_integer_object(1),
            },
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
            .constants = {
                make_integer_object(2), 
                make_integer_object(1),
            },
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
            .constants = {
                make_integer_object(1), 
            }, 1,
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
            {
                make_integer_object(1),
                make_integer_object(2),
            }, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_GREATER_THAN),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 < 2", 
            {
                make_integer_object(1),
                make_integer_object(2),
            }, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_LESS_THAN),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 == 2", 
            {
                make_integer_object(1),
                make_integer_object(2),
            }, 2,
            {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_EQUAL),
                make_instruction(OPCODE_POP),
            }, 4
        },
        {
            "1 != 2", 
            {
                make_integer_object(1),
                make_integer_object(2),
            }, 2,
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
            .constants = {
                make_integer_object(10),
                make_integer_object(3333),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_TRUE),              // 0000
                make_instruction(OPCODE_JUMP_NOT_TRUE, 10),  // 0001
                make_instruction(OPCODE_CONST, 0),          // 0004
                make_instruction(OPCODE_JUMP, 11),          // 0007
                make_instruction(OPCODE_NULL),              // 0010
                make_instruction(OPCODE_POP),               // 0011
                make_instruction(OPCODE_CONST, 1),          // 0012
                make_instruction(OPCODE_POP),               // 0015
            }, 8
        },
        {
            .input = "if (true) { 10; } else { 20; }; 3333;",
            .constants = {
                make_integer_object(10),
                make_integer_object(20),
                make_integer_object(3333),
            }, 3,
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

void test_global_let_statements() {
    TESTNAME(__FUNCTION__);

    struct compiler_test_case tests[] = {
        {
            .input = "let one = 1; let two = 2;",
            .constants = {
                make_integer_object(1),
                make_integer_object(2),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_SET_GLOBAL, 1),
            }, 4,
        },
        {
            .input = "let one = 1; one;",
            .constants = {
                make_integer_object(1),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_POP),
            }, 4,
        },
        {
            .input = "let one = 1; let two = one; two;",
            .constants = {
                make_integer_object(1),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_SET_GLOBAL, 1),
                make_instruction(OPCODE_GET_GLOBAL, 1),
                make_instruction(OPCODE_POP),
            }, 6,
        }
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void test_functions() {
    TESTNAME(__FUNCTION__);

    {
        struct instruction *fn_body[] = {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_ADD),
            make_instruction(OPCODE_RETURN_VALUE),
        };
        struct compiler_test_case t = {
            .input = "fn() { return 5 + 10 }",
            .constants = {
                make_integer_object(5),
                make_integer_object(10),
                make_compiled_function_object(flatten_instructions_array(fn_body, 4)),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
            }, 2,
        };
        run_compiler_test(t);
   }
   {
        struct instruction *fn_body[] = {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_ADD),
            make_instruction(OPCODE_RETURN_VALUE),
        };
        struct compiler_test_case t = {
            .input = "fn() { 5 + 10 }",
            .constants = {
                make_integer_object(5),
                make_integer_object(10),
                make_compiled_function_object(flatten_instructions_array(fn_body, 4)),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
            }, 2,
        };
        run_compiler_test(t);
    }
    {
        struct instruction *fn_body[] = {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_POP),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_RETURN_VALUE),
        };
        struct compiler_test_case t = {
            .input = "fn() { 1; 2 }",
            .constants = {
                make_integer_object(1),
                make_integer_object(2),
                make_compiled_function_object(flatten_instructions_array(fn_body, 4)),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
            }, 2,
        };
        run_compiler_test(t);
    }
    {
        struct compiler_test_case t = {
            .input = "fn() {}",
            .constants = {
                make_compiled_function_object(make_instruction(OPCODE_RETURN)),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_POP),
            }, 2,
        };
        run_compiler_test(t);
    }
}

void test_compiler_scopes() {
    TESTNAME(__FUNCTION__);
    struct compiler_scope scope;
    struct compiler *compiler = compiler_new();
    assertf(compiler->scope_index == 0, "wrong scope index: expected %d, got %d", 0, compiler->scope_index);
    compiler_emit(compiler, OPCODE_MULTIPLY);
    compiler_enter_scope(compiler);
    assertf(compiler->scope_index == 1, "wrong scope index: expected %d, got %d", 1, compiler->scope_index);
    compiler_emit(compiler, OPCODE_SUBTRACT);
    scope = compiler_current_scope(compiler);
    assertf(scope.instructions->size == 1, "wrong instruction size in scope: expected %d, got %d", 1, scope.instructions->size);
    assertf(scope.last_instruction.opcode == OPCODE_SUBTRACT, "wrong last instruction in scope: expected %d, got %d", OPCODE_SUBTRACT, scope.last_instruction.opcode);
    compiler_leave_scope(compiler);
    assertf(compiler->scope_index == 0, "wrong scope index: expected %d, got %d", 0, compiler->scope_index);
    compiler_emit(compiler, OPCODE_ADD);
    scope = compiler_current_scope(compiler);
    assertf(scope.instructions->size == 2, "wrong instruction size in scope: expected %d, got %d", 2, scope.instructions->size);
    assertf(scope.last_instruction.opcode == OPCODE_ADD, "wrong last instruction in scope: expected %d, got %d", OPCODE_ADD, scope.last_instruction.opcode);
    assertf(scope.previous_instruction.opcode == OPCODE_MULTIPLY, "wrong previous instruction in scope: expected %d, got %d", OPCODE_MULTIPLY, scope.previous_instruction.opcode);
    compiler_free(compiler);
}

void test_function_calls() {
    TESTNAME(__FUNCTION__);

    {
        struct instruction *fn_body[] = {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        };
        struct compiler_test_case t = {
            .input = "fn() { 24 }();",
            .constants = {
                make_integer_object(24),
                make_compiled_function_object(flatten_instructions_array(fn_body, 2)),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_CALL),
                make_instruction(OPCODE_POP),
            }, 3,
        };
        run_compiler_test(t);
   }
   {
        struct instruction *fn_body[] = {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        };
        struct compiler_test_case t = {
            .input = "let noArg = fn() { 24 }; noArg();",
            .constants = {
                make_integer_object(24),
                make_compiled_function_object(flatten_instructions_array(fn_body, 2)),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_CALL),
                make_instruction(OPCODE_POP),
            }, 5,
        };
        run_compiler_test(t);
   }
}

int main() {
    test_integer_arithmetic();
    test_boolean_expressions();
    test_conditionals();
    test_global_let_statements();
    test_compiler_scopes();
    test_functions();
    test_function_calls();
    printf("\x1b[32mAll compiler tests passed!\033[0m\n");
}