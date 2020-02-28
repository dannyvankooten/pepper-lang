#ifndef CODE_H 
#define CODE_H

#include <stdlib.h>

#define MAX_OP_SIZE 16

enum opcode {
    OP_CONST = 1,
};

struct definition {
    char name[64];
    unsigned int operand_widths[MAX_OP_SIZE];
};

struct instruction {
    unsigned char *bytes;
    size_t cap;
    size_t size;
};

struct definition lookup(enum opcode opcode);
struct instruction *make_instruction(enum opcode opcode, int operands[]);

#endif