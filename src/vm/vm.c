#include <stdlib.h>
#include <err.h>
#include "vm.h"

struct vm *make_vm(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    vm->stack_pointer = 0;
    
    // TODO: Dereference here
    vm->instructions = bc->instructions;
    vm->constants = bc->constants;
    return vm;
}

void vm_stack_push(struct vm *vm, struct object *obj) {
    if (vm->stack_pointer >= STACK_SIZE) {
        err(1, "stack overflow");
    }

    vm->stack[vm->stack_pointer++] = obj;
}

int vm_run(struct vm *vm) {
    // TODO implement
    for (int ip=0; ip < vm->instructions->size; ip++) {
        enum opcode opcode = vm->instructions->bytes[ip];
        switch (opcode) {
            case OPCODE_CONST: {
                int idx = read_bytes(vm->instructions->bytes, ip+1, 2);
                ip += 2;
                vm_stack_push(vm, vm->constants->values[idx]);
                break;
            }
        }
    }
    return 0;
}

struct object *vm_stack_top(struct vm *vm) {
    if  (vm->stack_pointer == 0) {
        return NULL;
    }

    return vm->stack[vm->stack_pointer-1];
}