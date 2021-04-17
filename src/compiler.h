#pragma once 

#include "opcode.h"
#include "object.h"
#include "symbol_table.h"

struct emitted_instruction {
    enum opcode opcode;
    uint32_t position;
};

struct compiler_scope {
    struct instruction *instructions;
    struct emitted_instruction last_instruction;
    struct emitted_instruction previous_instruction;
};

struct compiler {
    struct object_list *constants;
    struct symbol_table *symbol_table;
    uint32_t scope_index;
    struct compiler_scope scopes[64];
};

struct compiler *compiler_new();
struct compiler *compiler_new_with_state(struct symbol_table *t, struct object_list *constants);
void compiler_free(struct compiler *c);
int compile_program(struct compiler *compiler, const struct program *program);
struct bytecode *get_bytecode(struct compiler *c);
void concat_instructions(struct instruction *ins1, struct instruction *ins2);
const char *compiler_error_str(const int err);
uint32_t compiler_emit(struct compiler *c, const enum opcode opcode, ...);
void compiler_enter_scope(struct compiler *c);
struct instruction *compiler_leave_scope(struct compiler *c);
struct compiler_scope compiler_current_scope(struct compiler *c);