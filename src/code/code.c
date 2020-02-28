#include <stdlib.h>
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

// TODO: Should we keep op_size in definition struct?
struct instruction make(enum opcode opcode, int ops[MAX_OP_SIZE], unsigned int op_size) {
    struct instruction i = {.size = 0};
    struct definition def = lookup(opcode);

    // write opcode to first byte
    i.values[i.size++] = opcode;

    // write operands to remaining bytes
    for (int op_idx = 0; op_idx < op_size; op_idx++) {
        for (int byte_idx = def.operand_widths[op_idx]-1; byte_idx >= 0; byte_idx--) {
            i.values[i.size++] = ops[op_idx] >> (byte_idx * 8) & 0xff;
        }
        
    }
    
    return i;
}