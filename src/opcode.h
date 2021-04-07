#ifndef CODE_H 
#define CODE_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#define MAX_OP_SIZE 16

enum opcode {
    OPCODE_CONST = 0,
    OPCODE_POP,
    OPCODE_ADD,
    OPCODE_SUBTRACT,
    OPCODE_MULTIPLY,
    OPCODE_DIVIDE,
    OPCODE_TRUE,
    OPCODE_FALSE,
    OPCODE_EQUAL,
    OPCODE_NOT_EQUAL,
    OPCODE_GREATER_THAN,
    OPCODE_LESS_THAN,
    OPCODE_MINUS,
    OPCODE_BANG,
    OPCODE_JUMP,
    OPCODE_JUMP_NOT_TRUE,
    OPCODE_NULL,
    OPCODE_GET_GLOBAL,
    OPCODE_SET_GLOBAL,
    OPCODE_CALL,
    OPCODE_RETURN_VALUE,
    OPCODE_RETURN,
    OPCODE_GET_LOCAL,
    OPCODE_SET_LOCAL,
};

struct definition {
    char name[64];
    size_t operands;
    size_t operand_widths[MAX_OP_SIZE];
};

struct instruction {
    uint8_t *bytes;
    size_t cap;
    size_t size;
};

struct bytecode {
    struct instruction *instructions;
    struct object_list *constants;
};

char *opcode_to_str(enum opcode opcode);
struct definition lookup(enum opcode opcode);
struct instruction *make_instruction(enum opcode opcode, ...);
struct instruction *make_instruction_va(enum opcode opcode, va_list operands);
void free_instruction(struct instruction *ins);
struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size);
char *instruction_to_str(struct instruction *ins);
size_t read_operands(size_t dest[MAX_OP_SIZE], struct definition def, struct instruction *ins, size_t offset);
int read_bytes(uint8_t *bytes, uint8_t len);


#define read_uint8(bytes) (bytes)[0]
#define read_uint16(bytes) ((bytes)[0] << 8) + (bytes)[1]


#endif