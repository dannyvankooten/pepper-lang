#pragma once 

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>

#define MAX_OP_SIZE 16
#define read_uint8(b) ((b)[0])
#define read_uint16(b) (((b)[0] << 8) + ((b)[1]))

enum opcode {
    OPCODE_CONST = 0,
    OPCODE_POP,
    OPCODE_ADD,
    OPCODE_SUBTRACT,
    OPCODE_MULTIPLY,
    OPCODE_DIVIDE,
    OPCODE_MODULO,
    OPCODE_TRUE,
    OPCODE_FALSE,
    OPCODE_EQUAL,
    OPCODE_NOT_EQUAL,
    OPCODE_GREATER_THAN,
    OPCODE_GREATER_THAN_OR_EQUALS,
    OPCODE_LESS_THAN,
    OPCODE_LESS_THAN_OR_EQUALS,
    OPCODE_AND,
    OPCODE_OR,
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
    OPCODE_GET_BUILTIN,
    OPCODE_ARRAY,
    OPCODE_INDEX_GET,
    OPCODE_INDEX_SET,
    OPCODE_SLICE,
    OPCODE_HALT,
};

struct definition {
    const char name[32];
    uint8_t operands;
    uint8_t operand_widths[8];
};

struct instruction {
    uint8_t *bytes;
    unsigned cap;
    unsigned size;
};

struct bytecode {
    struct instruction *instructions;
    struct object_list *constants;
};

const char *opcode_to_str(enum opcode opcode);
struct definition lookup(enum opcode opcode);
struct instruction *make_instruction(enum opcode opcode, ...);
struct instruction *make_instruction_va(enum opcode opcode, va_list operands);
struct instruction *copy_instructions(const struct instruction *a);
void free_instruction(struct instruction *ins);
struct instruction *flatten_instructions_array(struct instruction *arr[], unsigned size);
char *instruction_to_str(struct instruction *ins);
unsigned read_operands(unsigned dest[], struct definition def, const struct instruction *ins, uint32_t offset);
