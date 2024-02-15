#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "compiler.h"
#include "object.h"
#include "opcode.h"
#include "parser.h"
#include "symbol_table.h"
#include "builtins.h"

enum {
    COMPILE_SUCCESS = 0,
    COMPILE_ERR_UNKNOWN_OPERATOR,
    COMPILE_ERR_UNKNOWN_EXPR_TYPE,
    COMPILE_ERR_UNKNOWN_IDENT,
    COMPILE_ERR_PREVIOUSLY_DECLARED,
};

const uint16_t JUMP_PLACEHOLDER_BREAK = 9999;
const uint16_t JUMP_PLACEHOLDER_CONTINUE = 9998;

static int compile_statement(struct compiler *compiler, const struct statement *statement);
static int compile_expression(struct compiler *compiler, const struct expression *expression);

struct compiler *compiler_new(void) {
    struct compiler *c = malloc(sizeof *c);
    assert(c != NULL);
    struct compiler_scope scope;
    scope.instructions = malloc(sizeof *scope.instructions);
    assert(scope.instructions != NULL);
    scope.instructions->cap = 2048; 
    scope.instructions->bytes = calloc(scope.instructions->cap, sizeof *scope.instructions->bytes);
    assert(scope.instructions->bytes != NULL);
    scope.instructions->size = 0;
    c->constants = make_object_list(64);

    c->symbol_table = symbol_table_new();
    define_builtins(c->symbol_table);

    // initialize scopes
    for (uint32_t i=0; i < 64; i++) {
        c->scopes[i].instructions = NULL;
    }
    c->scopes[0] = scope;
    c->scope_index = 0;
    return c;
}

struct compiler *compiler_new_with_state(struct symbol_table *t, struct object_list *constants) {
    struct compiler *c = compiler_new();
    symbol_table_free(c->symbol_table);
    c->symbol_table = t;
	define_builtins(c->symbol_table);
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
const char *compiler_error_str(int err) {
    static const char *error_messages[] = {
        "Success",
        "Unknown operator",
        "Unknown expression type",
        "Undefined variable",
        "Redeclaration of variable",
    };
    return error_messages[err];
}

struct compiler_scope compiler_current_scope(struct compiler *c) {
    return c->scopes[c->scope_index];
}

struct instruction *compiler_current_instructions(struct compiler *c) {
    struct compiler_scope scope = compiler_current_scope(c);
    return scope.instructions;
}

static uint32_t
add_constant(struct compiler *c, struct object obj) {
    c->constants->values[c->constants->size++] = obj;
    return c->constants->size - 1;
}

static void compiler_set_last_instruction(struct compiler *c, enum opcode opcode, uint32_t pos) {
    struct emitted_instruction previous = compiler_current_scope(c).last_instruction;
    struct emitted_instruction last = {
        .position = pos,
        .opcode = opcode,
    };
    c->scopes[c->scope_index].previous_instruction = previous;
    c->scopes[c->scope_index].last_instruction = last;
}

static void compiler_remove_last_instruction(struct compiler *c) {
    /* set instruction pointer back to position of last emitted instruction */
    c->scopes[c->scope_index].instructions->size = c->scopes[c->scope_index].last_instruction.position;
    c->scopes[c->scope_index].last_instruction = c->scopes[c->scope_index].previous_instruction;
}

static void compiler_replace_instruction(struct compiler *c, uint32_t pos, struct instruction *ins) {
    for (uint32_t i=0; i < ins->size; i++) {
        c->scopes[c->scope_index].instructions->bytes[pos + i] = ins->bytes[i];
    }

    free_instruction(ins);
}

static void compiler_replace_last_instruction(struct compiler *c, struct instruction *ins) {
    uint32_t pos = compiler_current_scope(c).last_instruction.position;
    enum opcode first = ins->bytes[0];
    compiler_replace_instruction(c, pos, ins);
    compiler_set_last_instruction(c, first, pos);
}

static bool compiler_last_instruction_is(struct compiler *c, enum opcode opcode) {
    if (c->scopes[c->scope_index].instructions->size == 0) {
        return false;
    }

    return c->scopes[c->scope_index].last_instruction.opcode == opcode;
}

static void compiler_change_operand(struct compiler *c, uint32_t pos, uint16_t operand) {
    enum opcode opcode = c->scopes[c->scope_index].instructions->bytes[pos];
    for (int8_t byte_idx = lookup(opcode).operand_widths[0] - 1, i = 1; byte_idx >= 0; byte_idx--, i++) {
        c->scopes[c->scope_index].instructions->bytes[pos + i] = (uint8_t) (operand >> (byte_idx * 8));
    }
}

static uint32_t compiler_emit_va(struct compiler *c, enum opcode opcode, va_list operands) {
    struct definition def = lookup(opcode);  
    struct instruction *cins = compiler_current_instructions(c);

    if (cins->size + def.operands * 3 >= cins->cap) {
        cins->cap *= 2;
        cins->bytes = realloc(cins->bytes, cins->cap * sizeof(*cins->bytes));
        assert(cins->bytes != NULL);
    }

    uint32_t pos = cins->size;

    // write opcode to bytecode
    cins->bytes[cins->size++] = opcode;

    // write operands to bytecode
    for (uint8_t op_idx = 0; op_idx < def.operands; op_idx++) {
        int64_t operand = va_arg(operands, int64_t);
        for (int8_t byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            cins->bytes[cins->size++] = (uint8_t) (operand >> (byte_idx * 8));
        }
    }

    compiler_set_last_instruction(c, opcode, pos);
    return pos;
}

uint32_t compiler_emit(struct compiler *c, enum opcode opcode, ...) {
    va_list args;
    va_start(args, opcode);
    uint32_t pos = compiler_emit_va(c, opcode, args);
    va_end(args);
    return pos;
}

int
compile_program(struct compiler *compiler, const struct program *program) {
    int err;
    for (uint32_t i=0; i < program->size; i++) {
        err = compile_statement(compiler, &program->statements[i]);
        if (err) return err;
    }

    // end every program with OPCODE_HALT so we can include it in the lookup table
    // vs. checking ip on every iteration
    compiler_emit(compiler, OPCODE_HALT);

    return 0;
}

static int
compile_block_statement(struct compiler *compiler, const struct block_statement *block) {
    int err;
    for (uint32_t i=0; i < block->size; i++) {
        err = compile_statement(compiler, &block->statements[i]);
        if (err) return err;
    }

    return 0;
}

static int
compile_statement(struct compiler *c, const struct statement *stmt) {
    int err;
    switch (stmt->type) {
        case STMT_EXPR: {
            // empty expressions, eg in for loops
            if (stmt->value == NULL) {
                return 0;
            }

            err = compile_expression(c, stmt->value);
            if (err) return err;

            compiler_emit(c, OPCODE_POP);
        }
        break;

        case STMT_LET: {
            struct symbol *s = symbol_table_define(c->symbol_table, stmt->name.value);
            if (s == NULL) {
                return COMPILE_ERR_PREVIOUSLY_DECLARED;
            }

            if (stmt->value != NULL) {
                err = compile_expression(c, stmt->value);
                if (err) return err;
            } else {
                // declaration without value
                // Eg: let foo;
                compiler_emit(c, OPCODE_NULL);
            }
            compiler_emit(c, s->scope == SCOPE_GLOBAL ? OPCODE_SET_GLOBAL : OPCODE_SET_LOCAL, s->index);
        }
        break;

        case STMT_RETURN: {
            // return statements have an optional value expression
            if (stmt->value == NULL) {
                return 0;
            }

            err = compile_expression(c, stmt->value);
            if (err) return err;
            compiler_emit(c, OPCODE_RETURN_VALUE);
        }
        break;

        case STMT_BREAK:
            // TODO: Validate that we're inside a loop. Or is that the parser's job?
            compiler_emit(c, OPCODE_NULL);
            compiler_emit(c, OPCODE_JUMP, JUMP_PLACEHOLDER_BREAK);
        break;

        case STMT_CONTINUE:
            compiler_emit(c, OPCODE_NULL);
            compiler_emit(c, OPCODE_JUMP, JUMP_PLACEHOLDER_CONTINUE);
        break;
    }

    return 0;
}

void compiler_change_jump_placeholders(struct compiler* c, uint32_t pos_start, uint32_t pos_end, int32_t placeholder_value, int32_t actual_value) {
    for (uint32_t p=pos_start; p < pos_end; ) {
        enum opcode op = c->scopes[c->scope_index].instructions->bytes[p];
        if (op == OPCODE_JUMP && read_uint16(&c->scopes[c->scope_index].instructions->bytes[p+1]) == placeholder_value) {
            compiler_change_operand(c, p, actual_value);
        }
        p += 1 + lookup(op).operand_widths[0];
    }
}

static int compile_infix_expression(struct compiler *c, const struct expression *expr) {
    int err = compile_expression(c, expr->infix.left);
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

        case OP_MODULO:
            compiler_emit(c, OPCODE_MODULO);
        break;

        case OP_GTE:
            compiler_emit(c, OPCODE_GREATER_THAN_OR_EQUALS);
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

        case OP_LTE:
            compiler_emit(c, OPCODE_LESS_THAN_OR_EQUALS);
        break;

        case OP_AND:
            compiler_emit(c, OPCODE_AND);
        break;

        case OP_OR:
            compiler_emit(c, OPCODE_OR);
        break;

        default:
            return COMPILE_ERR_UNKNOWN_OPERATOR;
        break;
    }

    return 0;
}

static int
compile_expression(struct compiler *c, const struct expression *expr) {
    int err;
    switch (expr->type) {
        case EXPR_INFIX: {
            return compile_infix_expression(c, expr);
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

        case EXPR_POSTFIX: {
            // load identifier on the stack
            err = compile_expression(c, expr->postfix.left);
            if (err) return err;

            // load again on the stack
            err = compile_expression(c, expr->postfix.left);
            if (err) return err;

            // load 1 on the stack
            compiler_emit(c, OPCODE_CONST, add_constant(c, make_integer_object(1)));

            switch (expr->postfix.operator) {
                case OP_ADD: 
                    compiler_emit(c, OPCODE_ADD);
                break;

                case OP_SUBTRACT: 
                    compiler_emit(c, OPCODE_SUBTRACT);
                break;

                default: 
                    return COMPILE_ERR_UNKNOWN_OPERATOR;
                break;
            }   

            // store result in identifier
            struct symbol *s = symbol_table_resolve(c->symbol_table, expr->postfix.left->ident.value);
            if (s == NULL) {
                return COMPILE_ERR_UNKNOWN_IDENT;
            }
            compiler_emit(c, s->scope == SCOPE_GLOBAL ? OPCODE_SET_GLOBAL : OPCODE_SET_LOCAL, s->index);        
        }
        break;

        case EXPR_IF: {
            err = compile_expression(c, expr->ifelse.condition);
            if (err) return err;

            /* we don't know where to jump yet, so we use 9999 as placeholder */
            uint32_t jump_if_not_true_pos = compiler_emit(c, OPCODE_JUMP_NOT_TRUE, 9999);

            err = compile_block_statement(c, expr->ifelse.consequence);
            if (err) { return err; }

            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_remove_last_instruction(c);
            }
            
            uint32_t jump_pos = compiler_emit(c, OPCODE_JUMP, 9999);

            /* now we know actual position to jump to, so change operand */
            uint32_t after_conseq_pos = c->scopes[c->scope_index].instructions->size;
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

            /* same story here, replace placeholder position with actual jump to position */
            uint32_t after_alternative_pos = c->scopes[c->scope_index].instructions->size;
            compiler_change_operand(c, jump_pos, after_alternative_pos);
        }
        break;

        case EXPR_INT: {
            struct object obj = make_integer_object(expr->integer);
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
            struct object obj = make_string_object(expr->string);
            compiler_emit(c, OPCODE_CONST, add_constant(c, obj));
        }
        break;

        case EXPR_IDENT: {
            struct symbol *s = symbol_table_resolve(c->symbol_table, expr->ident.value);
            if (s == NULL) {
                return COMPILE_ERR_UNKNOWN_IDENT;
            }

            switch (s->scope) {
                case SCOPE_GLOBAL:
                    compiler_emit(c, OPCODE_GET_GLOBAL, s->index);
                break;

                case SCOPE_LOCAL:
                    compiler_emit(c, OPCODE_GET_LOCAL, s->index);
                break;

                case SCOPE_BUILTIN:
                    compiler_emit(c, OPCODE_GET_BUILTIN, s->index);
                break;
            }
        }
        break;

        case EXPR_FUNCTION: {
            compiler_enter_scope(c);

            for (uint32_t i=0; i < expr->function.parameters.size; i++) {
                symbol_table_define(c->symbol_table, expr->function.parameters.values[i].value);
            }

            err = compile_block_statement(c, expr->function.body);
            if (err) return err;

            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_replace_last_instruction(c, make_instruction(OPCODE_RETURN_VALUE));
            } else if (!compiler_last_instruction_is(c, OPCODE_RETURN_VALUE)) {
                compiler_emit(c, OPCODE_RETURN);
            }

            uint32_t num_locals = c->symbol_table->size;
            struct instruction *ins = compiler_leave_scope(c);
            struct object obj = make_compiled_function_object(ins, num_locals);
            compiler_emit(c, OPCODE_CONST, add_constant(c, obj));
            free_instruction(ins);
        }
        break;

        case EXPR_CALL: {
            err = compile_expression(c, expr->call.function);
            if (err) return err;

            uint32_t i = 0;
            for (; i < expr->call.arguments.size; i++) {
                err = compile_expression(c, expr->call.arguments.values[i]);
                if (err) return err;
            }

            compiler_emit(c, OPCODE_CALL, i);
        }
        break;

        case EXPR_WHILE: {
            compiler_emit(c, OPCODE_NULL);

            uint32_t before_pos = c->scopes[c->scope_index].instructions->size;

            err = compile_expression(c, expr->while_loop.condition);
            if (err) return err;

            /* we don't know where to jump yet, so we use 9999 as placeholder */
            uint32_t jump_if_not_true_pos = compiler_emit(c, OPCODE_JUMP_NOT_TRUE, 9999);

            // pop null or last value from previous iteration
            compiler_emit(c, OPCODE_POP);

            uint32_t loop_start_pos = c->scopes[c->scope_index].instructions->size;
            err = compile_block_statement(c, expr->while_loop.body);
            if (err) { return err; }
            uint32_t loop_end_pos = c->scopes[c->scope_index].instructions->size;

            // leave last item on the stack
            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_remove_last_instruction(c);
            } else {
                compiler_emit(c, OPCODE_NULL);
            }

            /* jump back to beginning to re-evaluate condition */
            compiler_emit(c, OPCODE_JUMP, before_pos);

            /* now we know actual position to jump to, so change operand */
            uint32_t after_conseq_pos = c->scopes[c->scope_index].instructions->size;
            compiler_change_operand(c, jump_if_not_true_pos, after_conseq_pos);
            compiler_change_jump_placeholders(c, loop_start_pos, loop_end_pos, JUMP_PLACEHOLDER_BREAK, after_conseq_pos);
            compiler_change_jump_placeholders(c, loop_start_pos, loop_end_pos, JUMP_PLACEHOLDER_CONTINUE, before_pos);
        }
        break;

         case EXPR_FOR: {
            compiler_emit(c, OPCODE_NULL);

            err = compile_statement(c, &expr->for_loop.init);
            if (err) return err;

            uint32_t before_pos = c->scopes[c->scope_index].instructions->size;
            uint32_t jump_if_not_true_pos = 0;

            if (expr->for_loop.condition != NULL) {
                err = compile_expression(c, expr->for_loop.condition);
                if (err) return err;
           
                /* we don't know where to jump yet, so we use 9999 as placeholder */
                jump_if_not_true_pos = compiler_emit(c, OPCODE_JUMP_NOT_TRUE, 9999);
             }

            // pop null or last value from previous iteration
            compiler_emit(c, OPCODE_POP);

            uint32_t loop_start_pos = c->scopes[c->scope_index].instructions->size;
            err = compile_block_statement(c, expr->for_loop.body);
            if (err) { return err; }
            uint32_t loop_end_pos = c->scopes[c->scope_index].instructions->size;

            // leave last item on the stack
            if (compiler_last_instruction_is(c, OPCODE_POP)) {
                compiler_remove_last_instruction(c);
            } else {
                compiler_emit(c, OPCODE_NULL);
            }

            // run increment step
            uint32_t before_inc_pos = c->scopes[c->scope_index].instructions->size;
            err = compile_statement(c, &expr->for_loop.inc);
            if (err) return err;

            /* jump back to beginning to re-evaluate condition */
            compiler_emit(c, OPCODE_JUMP, before_pos);

            /* now we know actual position to jump to, so change operand */
            uint32_t after_conseq_pos = c->scopes[c->scope_index].instructions->size;
            if (expr->for_loop.condition != NULL) {
                compiler_change_operand(c, jump_if_not_true_pos, after_conseq_pos);
            }
            compiler_change_jump_placeholders(c, loop_start_pos, loop_end_pos, JUMP_PLACEHOLDER_BREAK, after_conseq_pos);
            compiler_change_jump_placeholders(c, loop_start_pos, loop_end_pos, JUMP_PLACEHOLDER_CONTINUE, before_inc_pos);
        }
        break;

        case EXPR_ARRAY:
            for (unsigned i=0; i < expr->array.size; i++) {
                err = compile_expression(c, expr->array.values[i]);
                if (err) return err;
            }
            compiler_emit(c, OPCODE_ARRAY, expr->array.size);
        break;

        case EXPR_SLICE: {
            err = compile_expression(c, expr->slice.left); 
            if (err) return err;
            
            if (expr->slice.start != NULL) {
                err = compile_expression(c, expr->slice.start); 
                if (err) return err;
            } else {
                compiler_emit(c, OPCODE_NULL);
            }

            if (expr->slice.end != NULL) {
                err = compile_expression(c, expr->slice.end); 
                if (err) return err;
            } else {
                compiler_emit(c, OPCODE_NULL);
            }
            
            compiler_emit(c, OPCODE_SLICE);
        }
        break;

        case EXPR_INDEX: {
            err = compile_expression(c, expr->index.left);
            if (err) return err;
            err = compile_expression(c, expr->index.index);
            if (err) return err;
            compiler_emit(c, OPCODE_INDEX_GET);
        break;
        }

        case EXPR_ASSIGN: {
            if (expr->assign.left->type == EXPR_IDENT) {
                struct symbol *s = symbol_table_resolve(c->symbol_table, expr->assign.left->ident.value);
                if (s == NULL) {
                    return COMPILE_ERR_UNKNOWN_IDENT;
                }

                err = compile_expression(c, expr->assign.value);
                if (err) return err;

                switch (s->scope) {
                    case SCOPE_GLOBAL:
                        compiler_emit(c, OPCODE_SET_GLOBAL, s->index);
                        compiler_emit(c, OPCODE_GET_GLOBAL, s->index);
                    break;

                    case SCOPE_LOCAL:
                        compiler_emit(c, OPCODE_SET_LOCAL, s->index);
                        compiler_emit(c, OPCODE_GET_LOCAL, s->index);
                    break;
                    default:
                        exit(1);
                        // TODO: emit error, can not redefine built-ins
                    break;
                }
            } else {
                err = compile_expression(c, expr->assign.left->index.left);
                if (err) return err;
                err = compile_expression(c, expr->assign.left->index.index);
                if (err) return err;
                err = compile_expression(c, expr->assign.value);
                if (err) return err;
                compiler_emit(c, OPCODE_INDEX_SET);    
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
    struct bytecode *b;
    b = malloc(sizeof *b);
    assert(b != NULL);
    b->instructions = compiler_current_instructions(c);
    b->constants = c->constants; // pointer, no copy
    return b;
}

void compiler_enter_scope(struct compiler *c) {
    struct compiler_scope scope;
    scope.instructions = malloc(sizeof *scope.instructions);
    assert(scope.instructions != NULL);
    scope.instructions->cap = 1024; // initial capacity of 1024 bytes
    scope.instructions->bytes = calloc(scope.instructions->cap, sizeof *scope.instructions->bytes);
    assert(scope.instructions->bytes != NULL);
    scope.instructions->size = 0;
    c->scopes[++c->scope_index] = scope;
    c->symbol_table = symbol_table_new_enclosed(c->symbol_table);
}

struct instruction *
compiler_leave_scope(struct compiler *c) {
    struct instruction *ins = c->scopes[c->scope_index].instructions;   
    struct symbol_table *t = c->symbol_table;
    c->symbol_table = c->symbol_table->outer;
    symbol_table_free(t);
    c->scope_index--;
    return ins;
}
