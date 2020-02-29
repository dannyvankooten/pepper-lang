#ifndef COMPILER_H 
#define COMPILER_H

#include "opcode.h"
#include "eval/object.h"

struct compiler {
    struct instruction *instructions;
    struct object_list *constants;
};

struct compiler *make_compiler();
void free_compiler(struct compiler *c);
int compile_program(struct compiler *compiler, struct program *program);
struct bytecode *get_bytecode(struct compiler *c);
void concat_instructions(struct instruction *ins1, struct instruction *ins2);

#endif