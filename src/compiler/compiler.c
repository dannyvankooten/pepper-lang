#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>

#include "compiler.h"

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

struct compiler *make_compiler() {
    struct compiler *c = malloc(sizeof *c);
    c->instructions = malloc(sizeof *c->instructions);
    c->constants = NULL;
    return c;
}

void free_compiler(struct compiler *c) {
    free(c->instructions);
    free(c->constants);
    free(c);
}

int
compile_program(struct compiler *compiler, struct program *program) {
    for (int i=0; i < program->size; i++) {
        int r = compile_statement(compiler, &program->statements[i]);
        if (r != 0) {
            return r;
        }
    }

    return 0;
}

int
compile_statement(struct compiler *compiler, struct statement *statement) {
    switch (statement->type) {
        // TODO: Handle STMT_LET 
        // TODO: Handle STMT_RETURN
        case STMT_EXPR: 
            return compile_expression(compiler, statement->value);
        break;
    }

    return 0;
}

size_t 
add_instruction(struct compiler *c, struct instruction *ins) {
    c->instructions->bytes = realloc(c->instructions->bytes, (c->instructions->size + ins->size ) * sizeof(*c->instructions->bytes));
    for (int i=0; i < ins->size; i++) {
        c->instructions->bytes[c->instructions->size++] = ins->bytes[i];
    }
    return c->instructions->size - 1;
}

size_t 
add_constant(struct compiler *c, struct object *obj) {
    if (!c->constants) {
        c->constants = make_object_list(16);
    }

    // TODO: Increase list capacity if needed

    c->constants->values[c->constants->size++] = obj;
    return c->constants->size - 1;
}

size_t compiler_emit(struct compiler *c, enum opcode opcode, ...) {
    va_list args;
    int i = 0;
    int operands[MAX_OP_SIZE];
    va_start(args, opcode);
        operands[i++] = va_arg(args, int);
    va_end(args);

    struct instruction *ins = make_instruction(opcode, operands);
    return add_instruction(c, ins);
}

int 
compile_expression(struct compiler *c, struct expression *expression) {
    switch (expression->type) {
        case EXPR_INFIX: 
            compile_expression(c, expression->infix.left);
            compile_expression(c, expression->infix.right);
        break; 

        case EXPR_INT: {
            struct object *obj = make_integer_object(expression->integer);
            compiler_emit(c,  OPCODE_CONST, add_constant(c, obj));
            break;
        }
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

