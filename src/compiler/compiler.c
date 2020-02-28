#include <stdlib.h>
#include "compiler.h"
#include <string.h> 

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

struct compiler *make_compiler() {
    struct compiler *c = malloc(sizeof *c);
    c->instructions = malloc(sizeof *c->instructions);
    c->constants = NULL;
    return c;
}

int
compile(struct compiler *compiler, struct program *program) {
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

int 
compile_expression(struct compiler *compiler, struct expression *expression) {
    switch (expression->type) {
        case EXPR_INFIX: 
            compile_expression(compiler, expression->infix.left);
            compile_expression(compiler, expression->infix.right);
        break; 

        case EXPR_INT: {
            struct object *obj = make_integer_object(expression->integer);
            size_t const_idx = add_constant(compiler, obj);
            struct instruction *ins = make_instruction(OP_CONST, (int[]) { const_idx });
            size_t ins_idx = add_instruction(compiler, ins);
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

