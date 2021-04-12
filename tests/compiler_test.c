#include "test_helpers.h"
#include "../src/compiler.h"

struct compiler_test_case {
    char *input;
    struct object *constants[16];
    uint32_t constants_size;
    struct instruction *instructions[16];
    uint32_t instructions_size;
};

void test_object(struct object *expected, struct object *actual) {
    assertf(actual != NULL, "expected object, got null");
    assertf(actual->type == expected->type, "invalid object type: expected %s, got %s", object_type_to_str(expected->type), object_type_to_str(actual->type));
    
    switch (expected->type) {
        case OBJ_INT:
            assertf(actual->value.integer == expected->value.integer, "invalid integer value: expected %d, got %d", expected->value.integer, actual->value.integer);
            free_object(expected);
        break;
        case OBJ_BOOL:
            assertf(actual->value.boolean == expected->value.boolean, "invalid boolean value: expected %d, got %d", expected->value.boolean, actual->value.boolean);
        break;
        case OBJ_COMPILED_FUNCTION: {
            char *expected_str = instruction_to_str(&expected->value.compiled_function->instructions);
            char *actual_str = instruction_to_str(&actual->value.compiled_function->instructions);
            assertf(expected->value.compiled_function->instructions.size == actual->value.compiled_function->instructions.size, "wrong instructions length: \nexpected\n\"%s\"\ngot\n\"%s\"", expected_str, actual_str);
            for (int i=0; i < expected->value.compiled_function->instructions.size; i++) {
                assertf(expected->value.compiled_function->instructions.bytes[i] == actual->value.compiled_function->instructions.bytes[i], "byte mismatch at pos %d: expected '%d', got '%d'\nexpected: %s\ngot: %s\n", i, expected->value.compiled_function->instructions.bytes[i], actual->value.compiled_function->instructions.bytes[i], expected_str, actual_str);
            }
            free(expected_str);
            free(actual_str);
            free_object(expected);
        }
        break;
        case OBJ_STRING: 
            assertf(strcmp(expected->value.string, actual->value.string) == 0, "invalid string value: expected %s, got %s", expected->value.string, actual->value.string);
            free_object(expected);
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

    free(concatted_str);
    free(bytecode_str);
    free(bytecode);
    free_instruction(concatted);
    free_program(program);
    compiler_free(compiler);
}

void run_compiler_tests(struct compiler_test_case tests[], uint32_t n) {
    for (int t=0; t < n; t++) {
       run_compiler_test(tests[t]);
    }
}

void integer_arithmetic() {

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
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 5,
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
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 5,
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
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 5,
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
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 5,
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
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 5,
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
                make_instruction(OPCODE_HALT),
            }, 4,
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void boolean_expressions() {

    struct compiler_test_case tests[] = {
        {
            .input = "true",
            .constants = {},
            .constants_size = 0,
            .instructions = {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            },
            .instructions_size = 3,
        },
        {
            "false",
            {}, 0,
            {
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            },
            3,
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
                make_instruction(OPCODE_HALT),
            }, 5
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
                make_instruction(OPCODE_HALT),
            }, 5
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
                make_instruction(OPCODE_HALT),
            }, 5
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
                make_instruction(OPCODE_HALT),
            }, 5
        },
        {
            "true == false", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_EQUAL),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 5
        },
        {
            "true != false", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_FALSE),
                make_instruction(OPCODE_NOT_EQUAL),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 5
        },
         {
            "!true", 
            {}, 0,
            {
                make_instruction(OPCODE_TRUE),
                make_instruction(OPCODE_BANG),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 4
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void if_statements() {

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
                make_instruction(OPCODE_HALT),
            }, 9
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
                make_instruction(OPCODE_HALT),
            }, 9
        },
    };
    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void global_let_statements() {

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
                make_instruction(OPCODE_HALT),
            }, 5,
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
                make_instruction(OPCODE_HALT),
            }, 5,
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
                make_instruction(OPCODE_HALT),
            }, 7,
        }
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void functions() {
    {
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_ADD),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 4);
        struct compiler_test_case t = {
            .input = "fn() { return 5 + 10 }",
            .constants = {
                make_integer_object(5),
                make_integer_object(10),
                make_compiled_function_object(fn_body, 0),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
   }
   {
       struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_ADD),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 4);
        struct compiler_test_case t = {
            .input = "fn() { 5 + 10 }",
            .constants = {
                make_integer_object(5),
                make_integer_object(10),
                make_compiled_function_object(fn_body, 0),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
    }
    {
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_POP),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 4);
        struct compiler_test_case t = {
            .input = "fn() { 1; 2 }",
            .constants = {
                make_integer_object(1),
                make_integer_object(2),
                make_compiled_function_object(fn_body, 0),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
    }
    {
        // test function without return value
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_RETURN),
        }, 1);
        struct compiler_test_case t = {
            .input = "fn() {}",
            .constants = {
                make_compiled_function_object(fn_body, 0),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
    }
}

void compiler_scopes() {
    struct compiler_scope scope;
    struct compiler *compiler = compiler_new();
    assertf(compiler->scope_index == 0, "wrong scope index: expected %d, got %d", 0, compiler->scope_index);
    struct symbol_table *globals = compiler->symbol_table;
    compiler_emit(compiler, OPCODE_MULTIPLY);
    compiler_enter_scope(compiler);
    assertf(compiler->scope_index == 1, "wrong scope index: expected %d, got %d", 1, compiler->scope_index);
    compiler_emit(compiler, OPCODE_SUBTRACT);
    scope = compiler_current_scope(compiler);
    assertf(scope.instructions->size == 1, "wrong instruction size in scope: expected %d, got %d", 1, scope.instructions->size);
    assertf(scope.last_instruction.opcode == OPCODE_SUBTRACT, "wrong last instruction in scope: expected %d, got %d", OPCODE_SUBTRACT, scope.last_instruction.opcode);
    assertf(compiler->symbol_table->outer == globals, "compiler did not enclose symbol table");
    // we have to manually free this instruction here because we don't hand it over to 
    // a compiled function
    struct instruction* ins = compiler_leave_scope(compiler);
    free_instruction(ins);
    assertf(compiler->symbol_table == globals, "compiler did not restore global symbol table");
    assertf(compiler->symbol_table->outer == NULL, "compiler modified global symbol table incorrectly");
    assertf(compiler->scope_index == 0, "wrong scope index: expected %d, got %d", 0, compiler->scope_index);
    compiler_emit(compiler, OPCODE_ADD);
    scope = compiler_current_scope(compiler);
    assertf(scope.instructions->size == 2, "wrong instruction size in scope: expected %d, got %d", 2, scope.instructions->size);
    assertf(scope.last_instruction.opcode == OPCODE_ADD, "wrong last instruction in scope: expected %d, got %d", OPCODE_ADD, scope.last_instruction.opcode);
    assertf(scope.previous_instruction.opcode == OPCODE_MULTIPLY, "wrong previous instruction in scope: expected %d, got %d", OPCODE_MULTIPLY, scope.previous_instruction.opcode);
    compiler_free(compiler);
}

void function_calls() {
    {
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct compiler_test_case t = {
            .input = "fn() { 24 }();",
            .constants = {
                make_integer_object(24),
                make_compiled_function_object(fn_body, 0),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_CALL, 0),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 4,
        };
        run_compiler_test(t);
   }
   {
       struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct compiler_test_case t = {
            .input = "let noArg = fn() { 24 }; noArg();",
            .constants = {
                make_integer_object(24),
                make_compiled_function_object(fn_body, 0),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_CALL, 0),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 6,
        };
        run_compiler_test(t);
   }
   { 
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_GET_LOCAL, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct compiler_test_case t = {
            .input = "let oneArg = fn(a) { a; }; oneArg(24);",
            .constants = {
                make_compiled_function_object(fn_body, 0),
                make_integer_object(24),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_CALL, 1),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 7,
        };
        run_compiler_test(t);
   }
   {
        struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_GET_LOCAL, 0),
            make_instruction(OPCODE_POP),
            make_instruction(OPCODE_GET_LOCAL, 1),
            make_instruction(OPCODE_POP),
            make_instruction(OPCODE_GET_LOCAL, 2),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 6);
        struct compiler_test_case t = {
            .input = "let manyArg = fn(a, b, c) { a; b; c; }; manyArg(24, 25, 26);",
            .constants = {
                make_compiled_function_object(fn_body, 0),
                make_integer_object(24),
                make_integer_object(25),
                make_integer_object(26),
            }, 4,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_CONST, 3),
                make_instruction(OPCODE_CALL, 3),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 9,
        };
        run_compiler_test(t);
   }
   {
        // test code with multiple functions to exercise scope logic (entering and leaving scopes)
        struct instruction *fn_one = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct instruction *fn_two = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 2),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct compiler_test_case t = {
            .input = "let one = fn() { 1; }; let two = fn() { 2; } one() + two();",
            .constants = {
                make_integer_object(1),
                make_compiled_function_object(fn_one, 0),
                make_integer_object(2),
                make_compiled_function_object(fn_two, 0),                
            }, 4,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_CONST, 3),
                make_instruction(OPCODE_SET_GLOBAL, 1),
                make_instruction(OPCODE_GET_GLOBAL, 0),
                make_instruction(OPCODE_CALL, 0),
                make_instruction(OPCODE_GET_GLOBAL, 1),
                make_instruction(OPCODE_CALL, 0),
                make_instruction(OPCODE_ADD),                
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 11,
        };
        run_compiler_test(t);
   }
}


void let_statement_scopes() {

    {
       struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_GET_GLOBAL, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 2);
        struct compiler_test_case t = {
            .input = "let num = 55;\nfn() { num }",
            .constants = {
                make_integer_object(55),
                make_compiled_function_object(fn_body, 0),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_SET_GLOBAL, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 5,
        };
        run_compiler_test(t);
   }
   {
       struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_SET_LOCAL, 0),
            make_instruction(OPCODE_GET_LOCAL, 0),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 4);
        struct compiler_test_case t = {
            .input = "fn() { let num = 55; num }",
            .constants = {
                make_integer_object(55),
                make_compiled_function_object(fn_body, 0),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
   }
   {
       struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
            make_instruction(OPCODE_CONST, 0),
            make_instruction(OPCODE_SET_LOCAL, 0),
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_SET_LOCAL, 1),
            make_instruction(OPCODE_GET_LOCAL, 0),
            make_instruction(OPCODE_GET_LOCAL, 1),
            make_instruction(OPCODE_ADD),
            make_instruction(OPCODE_RETURN_VALUE),
        }, 8);
        struct compiler_test_case t = {
            .input = "fn() { let a = 55; let b = 77; a + b }",
            .constants = {
                make_integer_object(55),
                make_integer_object(77),
                make_compiled_function_object(fn_body, 0),
            }, 3,
            .instructions = {
                make_instruction(OPCODE_CONST, 2),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        };
        run_compiler_test(t);
   }


}

void recursive_functions() {
    struct instruction *fn_body = flatten_instructions_array((struct instruction *[]) {
        make_instruction(OPCODE_GET_GLOBAL, 0),
        make_instruction(OPCODE_GET_LOCAL, 0),
        make_instruction(OPCODE_CONST, 0),
        make_instruction(OPCODE_SUBTRACT),
        make_instruction(OPCODE_CALL, 1),
        make_instruction(OPCODE_RETURN_VALUE),
        }, 6);
    struct compiler_test_case t = {
        .input = "let countdown = fn(x) { return countdown(x-1); }; countdown(1);",
        .constants = {
            make_integer_object(1),
            make_compiled_function_object(fn_body, 0),
            make_integer_object(1),
        }, 3,
        .instructions = {   
            make_instruction(OPCODE_CONST, 1),
            make_instruction(OPCODE_SET_GLOBAL, 0),
            make_instruction(OPCODE_GET_GLOBAL, 0),
            make_instruction(OPCODE_CONST, 2),
            make_instruction(OPCODE_CALL, 1),
            make_instruction(OPCODE_POP),
            make_instruction(OPCODE_HALT),
        }, 7,
    };
    run_compiler_test(t);
}

void string_expressions() {

    struct compiler_test_case tests[] = {
        {
            .input = "\"monkey\"",
            .constants = {
                make_string_object("monkey", NULL),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 3,
        },
        {
            .input = "\"mon\" + \"key\"",
            .constants = {
               make_string_object("mon", NULL),
               make_string_object("key", NULL),
            }, 2,
            .instructions = {
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CONST, 1),
                make_instruction(OPCODE_ADD),
                make_instruction(OPCODE_POP),
                make_instruction(OPCODE_HALT),
            }, 5,
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}

void builtin_functions() {
    struct compiler_test_case tests[] = {
        {
            .input = "len(\"monkey\")",
            .constants = {
                make_string_object("monkey", NULL),
            }, 1,
            .instructions = {
                make_instruction(OPCODE_GET_BUILTIN, 1),
                make_instruction(OPCODE_CONST, 0),
                make_instruction(OPCODE_CALL, 1),
                make_instruction(OPCODE_POP, 0),
                make_instruction(OPCODE_HALT),
            }, 5,
        },
    };

    run_compiler_tests(tests, ARRAY_SIZE(tests));
}



int main(int argc, char *argv[]) {    
    TEST(integer_arithmetic);
    TEST(boolean_expressions);
    TEST(if_statements);
    TEST(global_let_statements);
    TEST(compiler_scopes);
    TEST(functions);
    TEST(function_calls);
    TEST(let_statement_scopes);
    TEST(string_expressions);
    TEST(recursive_functions);
    TEST(builtin_functions);

    free_object_pool();
    free_object_list_pool();
}