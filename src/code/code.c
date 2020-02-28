#include <stdlib.h>
#include <stdarg.h>

#include "code.h"

struct definition lookup(enum opcode opcode) {
    switch (opcode) {
        case OP_CONST: {
            struct definition def = {
                .name = "OP_CONST",
                .operand_widths = {2},
                .operands = 1,
            };
            return def;
        }
        break;
    }

    struct definition def = {
        .name = "opcode undefined",
    };
    return def;
}

struct instruction *make_instruction(enum opcode opcode, int operands[]) {
    struct definition def = lookup(opcode);
    struct instruction *ins = malloc(sizeof *ins);

    ins->bytes = malloc(sizeof *ins->bytes * (def.operands + 1));
    ins->bytes[0] = opcode;
    ins->size = 1;
    ins->cap = 3;      

     // write operands to remaining bytes
    for (int op_idx = 0; op_idx < def.operands; op_idx++) {
        for (int byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            ins->bytes[ins->size++] = operands[op_idx] >> (byte_idx * 8);
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
    char str[265] = '\0';

    for (int i=0; i < ins->size; i++) {
        struct definition def = lookup(ins->bytes[i]);
        int operands[MAX_OP_SIZE];
        // TODO: Implement this
    }

    return str;
}

size_t read_operands(int dest[MAX_OP_SIZE], struct definition def, struct instruction *ins) {
    size_t bytes_read = 0;
    for (int i=0; i < def.operands; i++) {
        int max_shift = def.operand_widths[i] * 8;
        dest[i] = 0;
        // byte_idx is 1 here because we skip the opcode
        for (int byte_idx = 1; byte_idx <= def.operand_widths[i]; byte_idx++) {
            bytes_read++;
            dest[i] += ins->bytes[byte_idx] << (max_shift - 8 * byte_idx);;
        }
    }

    return bytes_read;
}