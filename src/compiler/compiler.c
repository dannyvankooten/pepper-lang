#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>

#include "compiler.h"

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

struct compiler *make_compiler() {
    struct compiler *c = malloc(sizeof *c);
    c->instructions = malloc(sizeof *c->instructions);
    c->instructions->bytes = malloc(1);
    c->instructions->size = 0;
    c->constants = make_object_list(64);
    return c;
}

void free_compiler(struct compiler *c) {
    free(c->instructions->bytes);
    free(c->instructions);
    free_object_list(c->constants);
    free(c);
}

char *compiler_error_str(int err) {
    static char *messages[] = {
        "Success",
        "Unknown operator",
        "Unknown expression type",
    };

    return messages[err];
}

size_t 
add_instruction(struct compiler *c, struct instruction *ins) {
    c->instructions->bytes = realloc(c->instructions->bytes, (c->instructions->size + ins->size ) * sizeof(*c->instructions->bytes));
    for (int i=0; i < ins->size; i++) {
        c->instructions->bytes[c->instructions->size++] = ins->bytes[i];
    }
    free_instruction(ins);
    return c->instructions->size - 1;
}

size_t 
add_constant(struct compiler *c, struct object *obj) {
    // TODO: Increase list capacity if needed

    c->constants->values[c->constants->size++] = obj;
    return c->constants->size - 1;
}

size_t compiler_emit(struct compiler *c, enum opcode opcode, ...) {
    va_list args;
    va_start(args, opcode);
    struct instruction *ins = make_instruction_va(opcode, args);
    va_end(args);
    return add_instruction(c, ins);
}

int
compile_program(struct compiler *compiler, struct program *program) {
    int err;
    for (int i=0; i < program->size; i++) {
        err = compile_statement(compiler, &program->statements[i]);
        if (err) {
            return err;
        }
    }

    return 0;
}

int
compile_statement(struct compiler *c, struct statement *statement) {
    int err;
    switch (statement->type) {
        // TODO: Handle STMT_LET 
        // TODO: Handle STMT_RETURN
        case STMT_EXPR: {
            err = compile_expression(c, statement->value);
            if (err) {
                return err;
            }

            compiler_emit(c, OPCODE_POP);
        }
        break;
    }

    return 0;
}


int 
compile_expression(struct compiler *c, struct expression *expr) {
    int err;
    switch (expr->type) {
        case EXPR_INFIX: {
            err = compile_expression(c, expr->infix.left);
            if (err) {
                return err;
            }

            err = compile_expression(c, expr->infix.right);
            if (err) {
                return err;
            }

            switch (expr->infix.operator) {
                case OP_ADD:
                    compiler_emit(c, OPCODE_ADD);
                break;
                case OP_SUBTRACT:
                    compiler_emit(c, OPCODE_SUBTRACT);
                break;
                case OP_MULTIPLY: 
                    compiler_emit(c, OPCODE_MULTIPLY);
                break;
                case OP_DIVIDE: 
                    compiler_emit(c, OPCODE_DIVIDE);
                break;
                default: 
                    return COMPILE_ERR_UNKNOWN_OPERATOR;
                break;
            }
        }
        break;   

        case EXPR_INT: {
            struct object *obj = make_integer_object(expr->integer);
            compiler_emit(c,  OPCODE_CONST, add_constant(c, obj));
            break;
        }

        case EXPR_BOOL: {
            if (expr->boolean) {
                compiler_emit(c, OPCODE_TRUE);
            } else {
                compiler_emit(c, OPCODE_FALSE);
            }
        }
        break;

        default:
            return COMPILE_ERR_UNKNOWN_EXPR_TYPE;
        break;
    }

    return 0;
}

struct bytecode *
get_bytecode(struct compiler *c) {
    struct bytecode *b = malloc(sizeof *b);
    b->instructions = c->instructions;
    b->constants = c->constants;
    return b;
}

