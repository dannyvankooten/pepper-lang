
#include <string.h> 
#include <stdint.h>

#include "../src/opcode.h"
#include "test_helpers.h"

static void test_make_instruction(void) {
    struct {
        enum opcode opcode;
        uint32_t operands[MAX_OP_SIZE];
        uint8_t expected[MAX_OP_SIZE];
        uint32_t expected_size;
    } tests[] = {
        {
            .opcode = OPCODE_CONST, 
            .operands = {65534}, 
            .expected = {OPCODE_CONST, 255, 254}, 3
        },
        {
            .opcode = OPCODE_ADD,
            .operands = {},
            .expected = {OPCODE_ADD}, 1
        },
        {
            .opcode = OPCODE_GET_LOCAL,
            .operands = {255},
            .expected = {OPCODE_GET_LOCAL, 255}, 2
        }
    };

    for (unsigned i=0; i < ARRAY_SIZE(tests); i++) {
        struct instruction *ins = make_instruction(tests[i].opcode, tests[i].operands[0]);
        
        assertf(ins->size == tests[i].expected_size, "wrong length: expected %d, got %d", tests[i].expected_size, ins->size);
        for (unsigned j=0; j < tests[i].expected_size; j++) {
            assertf(ins->bytes[j] == tests[i].expected[j], "[%d] invalid byte value at index %d: expected %d, got %d", i, j, tests[i].expected[j], ins->bytes[j]);
        }

        free_instruction(ins);
    }
}

static void test_instruction_string(void) {
    struct instruction *instructions[] = {
        make_instruction(OPCODE_ADD),
        make_instruction(OPCODE_GET_LOCAL, 1),
        make_instruction(OPCODE_CONST, 2),
        make_instruction(OPCODE_CONST, 65535)
    };

    char *expected_str = "0000 OpAdd | 0001 OpGetLocal 1 | 0003 OpConstant 2 | 0006 OpConstant 65535";
    struct instruction *ins = flatten_instructions_array(instructions, 4);
    char *str = instruction_to_str(ins);
    assertf(strcmp(expected_str, str) == 0, "wrong instruction string: expected \"%s\", got \"%s\"", expected_str, str);
    free_instruction(ins);
    free(str);
}

static void test_read_operands(void) {
    struct {
        enum opcode opcode;
        uint32_t operands[MAX_OP_SIZE];
        uint32_t bytes_read;
    } tests[] = {
        {OPCODE_CONST, {65535}, 2},
        {OPCODE_CONST, {1}, 2},
        {OPCODE_GET_LOCAL, {255}, 1},
    };

    for (unsigned t = 0; t < ARRAY_SIZE(tests); t++) {
        struct instruction *ins = make_instruction(tests[t].opcode, tests[t].operands[0]);
        struct definition def = lookup(tests[t].opcode);
        uint32_t operands[3] = {0};
        uint32_t bytes_read = read_operands(operands, def, ins, 0);
        assertf(bytes_read == tests[t].bytes_read, "wrong number of bytes read: expected %d, got %d", tests[t].bytes_read, bytes_read);
        for (uint32_t i=0; i < def.operands; i++) {
            assertf(tests[t].operands[i] == operands[i], "wrong operand: expected %d, got %d", tests[t].operands[i], operands[i]);
        }
        free_instruction(ins);
    }
}

static void test_read_bytes(void) {
    uint8_t bytes[] = {100, 20, 255};
    uint8_t v1 = read_uint8(bytes);
    assertf(v1 == 100, "read_bytes(uint8_t) failed: expected %d, got %d", 100, v1);
    uint16_t v2 = read_uint16(bytes);
    assertf(v2 == 25620, "read_bytes(uint16_5) failed: expected %d, got %d", 25620, v2);
}

int main(int argc, char *argv[]) {
    TEST(test_make_instruction);
    TEST(test_read_operands);
    TEST(test_instruction_string);
    TEST(test_read_bytes);
}
