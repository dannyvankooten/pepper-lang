#include "code/code.h"
#include "test_helpers.h"

#define ARRAY_SIZE(v) sizeof v / sizeof v[0]

void test_make_instruction() {
    struct {
        enum opcode opcode;
        int operands[16];
        unsigned char expected[16];
        size_t expected_size;
    } tests[] = {
        {OP_CONST, {65534}, {OP_CONST, 255, 254}, 3}
    };

    for (int i=0; i < ARRAY_SIZE(tests); i++) {
        struct instruction *ins = make_instruction(tests[i].opcode, tests[i].operands);
        
        assertf(ins->size == tests[i].expected_size, "wrong length: expected %d, got %d", tests[i].expected_size, ins->size);
        for (int j=0; j < tests[i].expected_size; j++) {
            assertf(ins->bytes[j] == tests[i].expected[j], "[%d] invalid byte value at index %d: expected %d, got %d", i, j, tests[i].expected[j], ins->bytes[j]);
        }

        free(ins);
    }
}

int main() {
    test_make_instruction();

    printf("\x1b[32mAll tests passed!\033[0m\n");
}