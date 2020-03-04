#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "opcode.h"


struct definition definitions[] = {
    {
        "OpConstant", 1, {2}
    },
    {
        "OpAdd", 0, {0}
    },
    {
        "OpPop", 0, {0}
    },
    {
        "OpSubtract", 0, {0}
    },
    {
        "OpMultiply", 0, {0}
    },
    {
        "OpDivide", 0, {0}
    },
    {
        "OpTrue", 0, {0}
    },
    {
        "OpFalse", 0, {0}
    },
    {
        "OpEqual", 0, {0}
    },
    {
        "OpNotEqual", 0, {0}
    },
    {
        "OpGreaterThan", 0, {0}
    },
    {
        "OpLessThan", 0, {0}
    },
    {
        "OpMinus", 0, {0}
    },
    {
        "OpBang", 0, {0}
    },
    {
        "OpJump", 1, {2}
    },
    {
        "OpJumpNotTrue", 1, {2}
    },
    {
        "OpNull", 0, {0}
    },
    {
        "OpGetGlobal", 1, {2}
    },
    {
        "OpSetGlobal", 1, {2}
    },
    {
        "OpCall", 0, {0},
    },
    {
        "OpReturnValue", 0, {0}
    },
    {
        "OpReturn", 0, {0}
    },
    {
        "OpGetLocal", 1, {1}
    },
    {
        "OpSetLocal", 1, {1}
    },
};

char *opcode_to_str(enum opcode opcode) {
    return definitions[opcode - 1].name;
}

struct definition lookup(enum opcode opcode) {
    return definitions[opcode - 1];
}

struct instruction *make_instruction_va(enum opcode opcode, va_list operands) {
    struct definition def = lookup(opcode);
    struct instruction *ins = malloc(sizeof *ins);
    
    ins->bytes = malloc(sizeof *ins->bytes * (def.operands + 1) * 3);
    ins->bytes[0] = opcode;
    ins->size = 1;
    ins->cap = (def.operands + 1);      

     // write operands to remaining bytes
    for (int op_idx = 0; op_idx < def.operands; op_idx++) {
        int operand = va_arg(operands, int);
        for (int byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            ins->bytes[ins->size++] = (unsigned char) (operand >> (byte_idx * 8) & 0xff);
        }
    }

    return ins;
}

struct instruction *make_instruction(enum opcode opcode, ...) {
    va_list args;
    va_start(args, opcode);
    struct instruction *ins = make_instruction_va(opcode, args);
    va_end(args);
    return ins;
}

void free_instruction(struct instruction *ins) {
    free(ins->bytes);
    free(ins);
}

struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size) {
    struct instruction *ins = arr[0];
    for (int i = 1; i < size; i++) {

        // TODO: Allocate all at once
        ins->bytes = realloc(ins->bytes, (ins->size + arr[i]->size ) * sizeof(*ins->bytes));
        for (int j=0; j < arr[i]->size; j++) {
            ins->bytes[ins->size++] = arr[i]->bytes[j];
        }

        free_instruction(arr[i]);
    }

    return ins;
}

char *instruction_to_str(struct instruction *ins) {
    char *buffer = malloc(ins->size * 32);
    int operands[MAX_OP_SIZE] = {0, 0};
    buffer[0] = '\0';

    for (int i=0; i < ins->size; i++) {
        struct definition def = lookup(ins->bytes[i]);
        size_t bytes_read = read_operands(operands, def, ins, i);
        
        if (i > 0) {
            strcat(buffer, "\n");
        }

        char str[512];
        switch (def.operands) {
            case 0:
                sprintf(str, "%04d %s", i, def.name);
            break;
            case 1:
                sprintf(str, "%04d %s %d", i, def.name, operands[0]);
            break;
            case 2:
                sprintf(str, "%04d %s %d %d", i, def.name, operands[0], operands[1]);
            break;
        }
        strcat(buffer, str);
        i += bytes_read;
    }


    return buffer;
}

int read_bytes(unsigned char *bytes, size_t offset, size_t len) {
    int sum = 0;
    int shift = (len - 1) * 8;

    for (int i = 0; i < len; i++) {
        sum += bytes[offset+i] << shift;
        shift -= 8;
    }

    return sum;
}

size_t read_operands(int *dest, struct definition def, struct instruction *ins, size_t offset) {
    size_t bytes_read = 0;

    // skip opcode
    offset += 1;

    // read bytes into dest array
    for (int i=0; i < def.operands; i++) {
        int sum = read_bytes(ins->bytes, offset, def.operand_widths[i]);
        dest[i] = sum;
        bytes_read += def.operand_widths[i];
    }

    return bytes_read;
}