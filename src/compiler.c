#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>
#include <assert.h>
#include "compiler.h"

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

struct compiler *compiler_new() {
    struct compiler *c = malloc(sizeof *c);
    c->instructions = malloc(sizeof *c->instructions);
    c->instructions->bytes = malloc(1);
    c->instructions->size = 0;
    c->constants = make_object_list(64);
    c->symbol_table = symbol_table_new();
    return c;
}

struct compiler *compiler_new_with_state(struct symbol_table *t, struct object_list *constants) {
    struct compiler *c = compiler_new();
    symbol_table_free(c->symbol_table);
    c->symbol_table = t;
    free_object_list(c->constants);
    c->constants = constants;
    return c;
}

void compiler_free(struct compiler *c) {
    free(c->instructions->bytes);
    free(c->instructions);
    free_object_list(c->constants);
    symbol_table_free(c->symbol_table);
    free(c);
}

/* TODO: We probably want dynamic error messages that includes parts of the program string */
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
    size_t pos = c->instructions->size;

    /* TODO: Use capacity here so we don't need to realloc on every new addition */
    c->instructions->bytes = realloc(c->instructions->bytes, (c->instructions->size + ins->size ) * sizeof(*c->instructions->bytes));
    for (int i=0; i < ins->size; i++) {
        c->instructions->bytes[c->instructions->size++] = ins->bytes[i];
    }
    free_instruction(ins);
    return pos;
}

size_t 
add_constant(struct compiler *c, struct object *obj) {
    // TODO: Dereference here?
    c->constants->values[c->constants->size++] = obj;
    return c->constants->size - 1;
}

void compiler_set_last_instruction(struct compiler *c, enum opcode opcode, size_t pos) {
    struct emitted_instruction previous = c->last_instruction;
    struct emitted_instruction last = {
        .position = pos,
        .opcode = opcode,
    };
    c->previous_instruction = previous;
    c->last_instruction = last;
}

void compiler_remove_last_instruction(struct compiler *c) {
    /* set instruction pointer back to position of last emitted instruction */
    c->instructions->size = c->last_instruction.position;
    c->last_instruction = c->previous_instruction;
}

void compiler_replace_instruction(struct compiler *c, size_t pos, struct instruction *ins) {
    for (int i=0; i < ins->size; i++) {
        c->instructions->bytes[pos + i] = ins->bytes[i];
    }
}

void compiler_change_operand(struct compiler *c, size_t pos, int operand) {
    enum opcode opcode = c->instructions->bytes[pos];
    struct instruction *new = make_instruction(opcode, operand);
    compiler_replace_instruction(c, pos, new);
}

size_t compiler_emit(struct compiler *c, enum opcode opcode, ...) {
    va_list args;
    va_start(args, opcode);
    struct instruction *ins = make_instruction_va(opcode, args);
    va_end(args);
    size_t pos = add_instruction(c, ins);
    compiler_set_last_instruction(c, opcode, pos);
    return pos;
}

int
compile_program(struct compiler *compiler, struct program *program) {
    int err;
    for (int i=0; i < program->size; i++) {
        err = compile_statement(compiler, &program->statements[i]);
        if (err) return err;
    }

    return 0;
}

int
compile_block_statement(struct compiler *compiler, struct block_statement *block) {
    int err;
    for (int i=0; i < block->size; i++) {
        err = compile_statement(compiler, &block->statements[i]);
        if (err) return err;
    }

    return 0;
}

int
compile_statement(struct compiler *c, struct statement *stmt) {
    int err;
    switch (stmt->type) {
        // TODO: Handle STMT_RETURN
        case STMT_EXPR: {
            err = compile_expression(c, stmt->value);
            if (err) return err;

            compiler_emit(c, OPCODE_POP);
        }
        break;

        case STMT_LET: {
            err = compile_expression(c, stmt->value);
            if (err) return err;

            struct symbol *s = symbol_table_define(c->symbol_table, stmt->name.value);
            compiler_emit(c, OPCODE_SET_GLOBAL, s->index);
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
            if (err) return err;

            err = compile_expression(c, expr->infix.right);
            if (err) return err;

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

                case OP_GT:
                    compiler_emit(c, OPCODE_GREATER_THAN);
                break;

                case OP_EQ: 
                    compiler_emit(c, OPCODE_EQUAL);
                break;

                case OP_NOT_EQ:
                    compiler_emit(c, OPCODE_NOT_EQUAL);
                break;

                case OP_LT:
                    compiler_emit(c, OPCODE_LESS_THAN);
                break;

                default: 
                    return COMPILE_ERR_UNKNOWN_OPERATOR;
                break;
            }
        }
        break;   

        case EXPR_PREFIX: {
            err = compile_expression(c, expr->prefix.right);
            if (err) return err;

            switch (expr->prefix.operator) {
                case OP_NEGATE: 
                    compiler_emit(c, OPCODE_BANG);
                break;

                case OP_SUBTRACT: 
                    compiler_emit(c, OPCODE_MINUS);
                break;

                default: 
                    return COMPILE_ERR_UNKNOWN_OPERATOR;
                break;
            }   
        }
        break;

        case EXPR_IF: {
            err = compile_expression(c, expr->ifelse.condition);
            if (err) return err;

            /* we don't know where to jump yet, so we use 9999 as placeholder */
            size_t jump_if_not_true_pos = compiler_emit(c, OPCODE_JUMP_NOT_TRUE, 9999);

            err = compile_block_statement(c, expr->ifelse.consequence);
            if (err) { return err; }

            if (c->last_instruction.opcode == OPCODE_POP) {
                compiler_remove_last_instruction(c);
            }

            size_t jump_pos = compiler_emit(c, OPCODE_JUMP, 9999);
            size_t after_conseq_pos = c->instructions->size;
            compiler_change_operand(c, jump_if_not_true_pos, after_conseq_pos);

            if (expr->ifelse.alternative) {
                err = compile_block_statement(c, expr->ifelse.alternative);
                if (err) return err; 

                if (c->last_instruction.opcode == OPCODE_POP) {
                    compiler_remove_last_instruction(c);
                }
            } else {
                compiler_emit(c, OPCODE_NULL);
            }

            size_t after_alternative_pos = c->instructions->size;
            compiler_change_operand(c, jump_pos, after_alternative_pos);
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

        case EXPR_IDENT: {
            struct symbol *s = symbol_table_resolve(c->symbol_table, expr->ident.value);
            assert(s != NULL);
            compiler_emit(c, OPCODE_GET_GLOBAL, s->index);
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

