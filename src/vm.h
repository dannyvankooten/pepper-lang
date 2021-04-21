#pragma once

#include <stdint.h>
#include "opcode.h"
#include "object.h"

#define FRAMES_SIZE 64
#define GLOBALS_SIZE 64
#define STACK_SIZE 256

enum result {
    VM_SUCCESS = 0,
    VM_ERR_INVALID_OP_TYPE,
    VM_ERR_INVALID_INT_OPERATOR,
    VM_ERR_INVALID_BOOL_OPERATOR,
    VM_ERR_OUT_OF_BOUNDS,
    VM_ERR_STACK_OVERFLOW,
    VM_ERR_INVALID_FUNCTION_CALL,
    VM_ERR_INVALID_INDEX_SOURCE,
};

struct frame {
    uint8_t *ip;
    struct compiled_function* fn;
    uint32_t base_pointer;    
};

struct vm {
    uint32_t stack_pointer;
    uint32_t frame_index;
    uint32_t nconstants;
    struct object stack[STACK_SIZE];
    struct object constants[GLOBALS_SIZE];
    struct frame frames[FRAMES_SIZE];
    struct object globals[GLOBALS_SIZE];
    struct object_list *heap;
};

struct vm *vm_new(struct bytecode *bc);
struct vm *vm_new_with_globals(struct bytecode *bc, struct object globals[STACK_SIZE]);
enum result vm_run(struct vm *vm);
struct object vm_stack_last_popped(struct vm *vm);
void vm_free(struct vm *vm);