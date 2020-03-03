#ifndef COMPILER_H 
#define COMPILER_H

#include "opcode.h"
#include "object.h"
#include "symbol_table.h"

#define COMPILE_ERR_UNKNOWN_OPERATOR 1
#define COMPILE_ERR_UNKNOWN_EXPR_TYPE 2

struct emitted_instruction {
    enum opcode opcode;
    size_t position;
};

struct compiler {
    struct instruction *instructions;
    struct object_list *constants;
    struct emitted_instruction last_instruction;
    struct emitted_instruction previous_instruction;
    struct symbol_table *symbol_table;
};

struct compiler *compiler_new();
struct compiler *compiler_new_with_state(struct symbol_table *t, struct object_list *constants);
void compiler_free(struct compiler *c);
int compile_program(struct compiler *compiler, struct program *program);
struct bytecode *get_bytecode(struct compiler *c);
void concat_instructions(struct instruction *ins1, struct instruction *ins2);
char *compiler_error_str(int err);

#endif