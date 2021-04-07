#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>
#include <assert.h>
#include "compiler.h"

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

struct compiler *compiler_new() {
    struct compiler *c = malloc(sizeof *c);
    struct compiler_scope scope;
    scope.instructions = malloc(sizeof *scope.instructions);
    scope.instructions->cap = 1024; // initial capacity of 1024 bytes
    scope.instructions->bytes = malloc(sizeof *scope.instructions->bytes * scope.instructions->cap);
    scope.instructions->size = 0;
    c->constants = make_object_list(64);
    c->symbol_table = symbol_table_new();
    c->scopes[0] = scope;
    c->scope_index = 0;
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
    free_instruction(c->scopes[0].instructions);
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

struct compiler_scope compiler_current_scope(struct compiler *c) {
    return c->scopes[c->scope_index];
}

struct instruction *compiler_current_instructions(struct compiler *c) {
    struct compiler_scope scope = compiler_current_scope(c);
    return scope.instructions;
}

size_t 
add_instruction(struct compiler *c, struct instruction *ins) {
    struct instruction *cins = compiler_current_instructions(c);
    size_t pos = cins->size;

    size_t new_size = cins->size + ins->size;
    if (new_size >= cins->cap) {
        cins->cap *= 2;
        cins->bytes = realloc(cins->bytes, cins->cap * sizeof(*cins->bytes));
    }

    for (size_t i=0; i < ins->size; i++) {
        cins->bytes[cins->size++] = ins->bytes[i];
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

void compiler_set_last_instruction(struct compiler *c, enum opcode opcode, unsigned int pos) {
    struct emitted_instruction previous = compiler_current_scope(c).last_instruction;
    struct emitted_instruction last = {
        .position = pos,
        .opcode = opcode,
    };
    c->scopes[c->scope_index].previous_instruction = previous;
    c->scopes[c->scope_index].last_instruction = last;
}

void compiler_remove_last_instruction(struct compiler *c) {
    /* set instruction pointer back to position of last emitted instruction */
    c->scopes[c->scope_index].instructions->size = c->scopes[c->scope_index].last_instruction.position;
    c->scopes[c->scope_index].last_instruction = c->scopes[c->scope_index].previous_instruction;
}

void compiler_replace_instruction(struct compiler *c, unsigned int pos, struct instruction *ins) {
    for (size_t i=0; i < ins->size; i++) {
        c->scopes[c->scope_index].instructions->bytes[pos + i] = ins->bytes[i];
    }
}

void compiler_replace_last_instruction(struct compiler *c, struct instruction *ins) {
    size_t pos = compiler_current_scope(c).last_instruction.position;
    compiler_replace_instruction(c, pos, ins);
    compiler_set_last_instruction(c, ins->bytes[0], pos);
}

bool compiler_last_instruction_is(struct compiler *c, enum opcode opcode) {
    if (c->scopes[c->scope_index].instructions->size == 0) {
        return false;
    }

    return c->scopes[c->scope_index].last_instruction.opcode == opcode;
}

void compiler_change_operand(struct compiler *c, size_t pos, int operand) {
    enum opcode opcode = c->scopes[c->scope_index].instructions->bytes[pos];
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
    for (size_t i=0; i < program->size; i++) {
        err = compile_statement(compiler, &program->statements[i]);
        if (err) return err;
    }

    return 0;
}

int
compile_block_statement(struct compiler *compiler, struct block_statement *block) {
    int err;
    for (size_t i=0; i < block->size; i++) {
        err = compile_statement(compiler, &block->statements[i]);
        if (err) return err;
    }

    return 0;
}

int
compile_statement(struct compiler *c, struct statement *stmt) {
    int err;
    switch (stmt->type) {
        case STMT_EXPR: {
            err = compile_expression(c, stmt->value);
            if (err) return err;

            compiler_emit(c, OPCODE_POP);
        }
        break;

        case STMT_LET: {
            struct symbol *s = symbol_table_define(c->symbol_table, stmt->name.value);
            err = compile_expression(c, stmt->value);
            if (err) return err;
            compiler_emit(c, s->scope == SCOPE_GLOBAL ? OPCODE_SET_GLOBAL : OPCODE_SET_LOCAL, s->index);
        }
        break;

        case STMT_RETURN: {
            err = compile_expression(c, stmt->value);
            if (err) return err;
            compiler_emit(c, OPCODE_RETURN_VALUE);
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

            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_remove_last_instruction(c);
            }

            size_t jump_pos = compiler_emit(c, OPCODE_JUMP, 9999);
            size_t after_conseq_pos = c->scopes[c->scope_index].instructions->size;
            compiler_change_operand(c, jump_if_not_true_pos, after_conseq_pos);

            if (expr->ifelse.alternative) {
                err = compile_block_statement(c, expr->ifelse.alternative);
                if (err) return err; 

                if (compiler_last_instruction_is(c, OPCODE_POP)) {
                    compiler_remove_last_instruction(c);
                }
            } else {
                compiler_emit(c, OPCODE_NULL);
            }

            size_t after_alternative_pos = c->scopes[c->scope_index].instructions->size;
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

        case EXPR_STRING: {
            // FIXME: Copy string here
            struct object *obj = make_string_object(expr->string, NULL);
            compiler_emit(c, OPCODE_CONST, add_constant(c, obj));
        }
        break;

        case EXPR_IDENT: {
            struct symbol *s = symbol_table_resolve(c->symbol_table, expr->ident.value);
            if (s != NULL) {
                compiler_emit(c, s->scope == SCOPE_GLOBAL ? OPCODE_GET_GLOBAL : OPCODE_GET_LOCAL, s->index);
            } else {
                compiler_emit(c, OPCODE_NULL);
            }
        }
        break;

        case EXPR_FUNCTION: {
            compiler_enter_scope(c);

            for (size_t i=0; i < expr->function.parameters.size; i++) {
                symbol_table_define(c->symbol_table, expr->function.parameters.values[i].value);
            }

            err = compile_block_statement(c, expr->function.body);
            if (err) return err;

            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_replace_last_instruction(c, make_instruction(OPCODE_RETURN_VALUE));
            } else if (!compiler_last_instruction_is(c, OPCODE_RETURN_VALUE)) {
                compiler_emit(c, OPCODE_RETURN);
            }

            size_t num_locals = c->symbol_table->size;
            struct instruction *ins = compiler_leave_scope(c);
            struct object *obj = make_compiled_function_object(ins, num_locals);
            compiler_emit(c, OPCODE_CONST, add_constant(c, obj));
        }
        break;

        case EXPR_CALL: {
            err = compile_expression(c, expr->call.function);
            if (err) return err;

            size_t i = 0;
            for (; i < expr->call.arguments.size; i++) {
                err = compile_expression(c, expr->call.arguments.values[i]);
                if (err) return err;
            }

            compiler_emit(c, OPCODE_CALL, i);
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
    b->instructions = compiler_current_instructions(c);
    // TODO: Copy object list here?
    b->constants = c->constants;
    return b;
}

void compiler_enter_scope(struct compiler *c) {
    c->scope_index++;

    struct compiler_scope scope;
    scope.instructions = malloc(sizeof *scope.instructions);
    scope.instructions->cap = 1024; // initial capacity of 1024 bytes
    scope.instructions->bytes = malloc(sizeof *scope.instructions->bytes * scope.instructions->cap);
    scope.instructions->size = 0;
    c->scopes[c->scope_index] = scope;

    c->symbol_table = symbol_table_new_enclosed(c->symbol_table);
}

struct instruction *
compiler_leave_scope(struct compiler *c) {
    struct instruction *ins = c->scopes[c->scope_index].instructions;
    // We transfer ownership of this scope's instructions to a compiled function
    // So we do not free it here.
    // free(c->scopes[c->scope_index].instructions->bytes);
    // free(c->scopes[c->scope_index].instructions);
    struct symbol_table *t = c->symbol_table;
    c->symbol_table = c->symbol_table->outer;
    symbol_table_free(t);

    c->scope_index--;
    return ins;
}
