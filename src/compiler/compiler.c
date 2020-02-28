#include <stdlib.h>
#include "compiler.h"
#include <string.h> 

int compile_statement(struct compiler *compiler, struct statement *statement);
int compile_expression(struct compiler *compiler, struct expression *expression);

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

int
compile_expression(struct compiler *compiler, struct expression *expression) {
    switch (expression->type) {
        case EXPR_INFIX: 
            compile_expression(compiler, expression->infix.left);
            compile_expression(compiler, expression->infix.right);
        break; 

        case EXPR_INT: {
            struct object *obj = make_integer_object(expression->integer);
            compiler->constants[compiler->constants_size++] = *obj;
            struct instruction ins = make(OP_CONST, (int[]) {compiler->constants_size - 1}, 1);
            compiler->instructions[compiler->instructions_size++] = ins;
            break;
        }
    }

    return 0;
}

struct bytecode *
get_bytecode(struct compiler *c) {
    struct bytecode *b = malloc(sizeof *b);

    memcpy(b->instructions, c->instructions, sizeof(c->instructions));
}

