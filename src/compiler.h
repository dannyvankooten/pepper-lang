#pragma once 

#include "opcode.h"
#include "object.h"
#include "symbol_table.h"

#define COMPILE_ERR_UNKNOWN_OPERATOR 1
#define COMPILE_ERR_UNKNOWN_EXPR_TYPE 2

struct emitted_instruction {
    enum opcode opcode;
    size_t position;
};

struct compiler_scope {
    struct instruction *instructions;
    struct emitted_instruction last_instruction;
    struct emitted_instruction previous_instruction;
};

struct compiler {
    struct object_list *constants;
    struct symbol_table *symbol_table;
    size_t scope_index;
    struct compiler_scope scopes[64];
};

struct compiler *compiler_new();
struct compiler *compiler_new_with_state(struct symbol_table *t, struct object_list *constants);
void compiler_free(struct compiler *c);
int compile_program(struct compiler *compiler, struct program *program);
struct bytecode *get_bytecode(struct compiler *c);
void concat_instructions(struct instruction *ins1, struct instruction *ins2);
const char *compiler_error_str(int err);
size_t compiler_emit(struct compiler *c, enum opcode opcode, ...);
void compiler_enter_scope(struct compiler *c);
struct instruction *compiler_leave_scope(struct compiler *c);
struct compiler_scope compiler_current_scope(struct compiler *c);