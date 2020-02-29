#ifndef VM_H 
#define VM_H 

#define STACK_SIZE 2048 

#include "compiler/opcode.h"
#include "eval/object.h"

struct vm {
    struct object_list *constants;
    struct instruction *instructions;
    
    // TODO: Should we store values on the stack?
    struct object *stack[STACK_SIZE];
    size_t stack_pointer;
};

struct vm *make_vm(struct bytecode *bc);
int vm_run(struct vm *vm);
struct object *vm_stack_top(struct vm *vm);

#endif 