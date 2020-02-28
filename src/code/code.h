#ifndef CODE_H 
#define CODE_H

#include <stdlib.h>

#define MAX_OP_SIZE 16

enum opcode {
    OP_CONST = 1,
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

struct definition lookup(enum opcode opcode);
struct instruction *make_instruction(enum opcode opcode, int operands[]);
struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size);
char *instruction_to_str(struct instruction *ins);
size_t read_operands(int dest[MAX_OP_SIZE], struct definition def, struct instruction *ins);

#endif