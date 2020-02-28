#ifndef CODE_H 
#define CODE_H

#define MAX_OP_SIZE 16

enum opcode {
    OP_CONST,
};

struct definition {
    char name[64];
    unsigned int operand_widths[MAX_OP_SIZE];
};

struct instruction {
    unsigned char values[MAX_OP_SIZE];
    unsigned int size;
};

struct definition lookup(enum opcode opcode);

// TODO: Should we keep op_size in definition struct?
struct instruction make(enum opcode opcode, int ops[], unsigned int op_size);

#endif