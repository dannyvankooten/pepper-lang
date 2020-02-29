#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "opcode.h"

struct definition lookup(enum opcode opcode) {
    switch (opcode) {
        case OPCODE_CONST: {
            struct definition def = {
                .name = "OpConstant",
                .operand_widths = {2},
                .operands = 1,
            };
            return def;
        }
        break;
        case OPCODE_ADD: {
            struct definition def = {
                .name = "OpAdd",
                .operands = 0,
            };
            return def;
        }
        break;
    }

    struct definition def;
    sprintf(def.name, "no definition for opcode: %d", opcode);
    return def;
}

struct instruction *make_instruction(enum opcode opcode, int operands[MAX_OP_SIZE]) {
    struct definition def = lookup(opcode);
    struct instruction *ins = malloc(sizeof *ins);

    ins->bytes = malloc(sizeof *ins->bytes * (def.operands + 1));
    ins->bytes[0] = opcode;
    ins->size = 1;
    ins->cap = (def.operands + 1);      

     // write operands to remaining bytes
    for (int op_idx = 0; op_idx < def.operands; op_idx++) {
        for (int byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            ins->bytes[ins->size++] = operands[op_idx] >> (byte_idx * 8) & 0xff;
        }
    }

    return ins;
}

struct instruction *flatten_instructions_array(struct instruction *arr[], size_t size) {
    struct instruction *ins = arr[0];
    for (int i = 1; i < size; i++) {
        ins->bytes = realloc(ins->bytes, (ins->size + arr[i]->size ) * sizeof(*ins->bytes));
        for (int j=0; j < arr[i]->size; j++) {
            ins->bytes[ins->size++] = arr[i]->bytes[j];
        }

        free(arr[i]);
    }
    return ins;
}

char *instruction_to_str(struct instruction *ins) {
    char *buffer = malloc(1024);
    buffer[0] = '\0';
    int operands[MAX_OP_SIZE] = {0};

    for (int i=0; i < ins->size; i++) {
        struct definition def = lookup(ins->bytes[i]);
        size_t bytes_read = read_operands(operands, def, ins, i);
        
        char str[256] = {'\0'};
        switch (def.operands) {
            case 1:
                sprintf(str, "%04d %s %d", i, def.name, operands[0]);
            break;

            // TODO: Add support for opcodes with more operands
        }
        strcat(buffer, str);

        i += bytes_read;
        if (i < (ins->size - 1)) {
            strcat(buffer, "\n");
        }
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