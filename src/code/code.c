#include <stdlib.h>
#include <stdarg.h>

#include "code.h"

struct definition lookup(enum opcode opcode) {
    switch (opcode) {
        case OP_CONST: {
            struct definition def = {
                .name = "OP_CONST",
                .operand_widths = {2},
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

    // TODO: Make size dynamic here, eg by placing it in definition struct
    size_t op_size = 1;

    ins->bytes = malloc(sizeof *ins->bytes * (op_size + 1));
    ins->bytes[0] = opcode;
    ins->size = 1;
    ins->cap = 3;      

     // write operands to remaining bytes
    for (int op_idx = 0; op_idx < op_size; op_idx++) {
        for (int byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            ins->bytes[ins->size++] = operands[op_idx] >> (byte_idx * 8) & 0xff;
        }
    }

    return ins;
}