#ifndef VM_H 
#define VM_H 

#define STACK_SIZE 2048 
#define MAX_GLOBALS 65536

#include "opcode.h"
#include "object.h"

struct frame {
    struct object *fn;
    size_t ip;
};

struct vm {
    struct object_list *constants;
    struct object_list *globals;

    struct frame frames[64];
    size_t frame_index;
    
    // TODO: Should we store values on the stack?
    struct object *stack[STACK_SIZE];
    size_t stack_pointer;
};



struct vm *vm_new(struct bytecode *bc);
struct vm *vm_new_with_globals(struct bytecode *bc, struct object_list *globals);
int vm_run(struct vm *vm);
struct object *vm_stack_last_popped(struct vm *vm);
struct object *vm_stack_pop(struct vm *vm);
void vm_free(struct vm *vm);

struct frame frame_new(struct object *obj);
struct instruction *frame_instructions(struct frame *f);

#endif 