#ifndef CODE_H 
#define CODE_H

#include <stdlib.h>
#include <stdarg.h>

#define MAX_OP_SIZE 16

enum opcode {
    OPCODE_CONST = 1,
    OPCODE_ADD,
    OPCODE_POP,
};

struct definition {
    char name[64];
    size_t operands;
    int operand_widths[MAX_OP_SIZE];
};

struct instruction {
    unsigned char *bytes;
    size_t cap;
    size_t size;
};

struct bytecode {
    struct instruction *instructions;
    struct object_list *constants;
};

struct definition lookup(enum opcode opcode);
struct instruction *make_instruction(enum opcode opcode, ...);
struct instruction *make_instruction_va(enum opcode opcode, va_list operands);
struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size);
char *instruction_to_str(struct instruction *ins);
size_t read_operands(int dest[MAX_OP_SIZE], struct definition def, struct instruction *ins, size_t offset);
int read_bytes(unsigned char *bytes, size_t offset, size_t len);
#endif