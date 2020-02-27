#include "code/code.c"
#include "test_helpers.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

void test_make() {
    struct {
        enum opcode opcode;
        int operands[16];
        int op_size;
        unsigned char expected[16];
        int instr_size;
    } tests[] = {
        {OP_CONST, {65534}, 1, {OP_CONST, 255, 254}, 3}
    };

    for (int i=0; i < ARRAY_SIZE(tests); i++) {
        struct instruction result = make(tests[i].opcode, tests[i].operands, tests[i].op_size);
        
        assertf(result.size == tests[i].instr_size, "wrong length: expected %d, got %d", tests[i].instr_size, result.size);
        for (int j=0; j < tests[i].instr_size; j++) {
            assertf(result.values[j] == tests[i].expected[j], "[%d] invalid byte value for bit %d: expected %d, got %d", i, j, tests[i].expected[j], result.values[j]);
        }
    }
}

int main() {
    test_make();

    printf("\x1b[32mAll tests passed!\033[0m\n");
}