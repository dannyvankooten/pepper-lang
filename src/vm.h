#ifndef VM_H 
#define VM_H 

#define STACK_SIZE 2048
#define MAX_GLOBALS 65536

#include "opcode.h"
#include "object.h"

struct frame {
    struct compiled_function fn;
    unsigned int ip;
    unsigned int base_pointer;
};

struct vm {
    struct frame frames[STACK_SIZE];
    unsigned int frame_index;
    
    struct object constants[STACK_SIZE];
    struct object globals[STACK_SIZE];
    struct object stack[STACK_SIZE];
    unsigned int stack_pointer;
};

extern const struct object obj_null;
extern const struct object obj_true;
extern const struct object obj_false;

struct vm *vm_new(struct bytecode *bc);
struct vm *vm_new_with_globals(struct bytecode *bc, struct object globals[STACK_SIZE]);
int vm_run(struct vm *vm);
struct object vm_stack_last_popped(struct vm *vm);
struct object vm_stack_pop(struct vm *vm);
void vm_free(struct vm *vm);

struct frame frame_new(struct object obj, unsigned int bp);
struct instruction *frame_instructions(struct frame *f);

#endif 