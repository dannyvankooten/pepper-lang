#include <stdlib.h>
#include <err.h>
#include "vm.h"

#define VM_ERR_INVALID_OP_TYPE 1
#define VM_ERR_INVALID_INT_OPERATOR 2
#define VM_ERR_OUT_OF_BOUNDS 3
#define VM_ERR_STACK_OVERFLOW 4

struct vm *vm_new(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    vm->stack_pointer = 0;
    
    // TODO: Dereference here?
    vm->globals = make_object_list(512);
    vm->instructions = bc->instructions;
    vm->constants = bc->constants;
    return vm;
}

struct vm *vm_new_with_globals(struct bytecode *bc, struct object_list *globals) {
    struct vm *vm = vm_new(bc);
    free_object_list(vm->globals);
    vm->globals = globals;
    return vm;
}

void vm_free(struct vm *vm) {
    // TODO: Free globals
    // TODO: Free objects on stack
    free(vm);
}

struct object *vm_stack_pop(struct vm *vm) {
    if (vm->stack_pointer == 0) {
        return NULL;
    }

    return vm->stack[--vm->stack_pointer];
}

int vm_stack_push(struct vm *vm, struct object *obj) {
    if (vm->stack_pointer >= STACK_SIZE) {
        err(VM_ERR_STACK_OVERFLOW, "stack overflow");
        return VM_ERR_STACK_OVERFLOW;
    }

    vm->stack[vm->stack_pointer++] = obj;
    return 0;
}

int vm_do_binary_integer_operation(struct vm *vm, enum opcode opcode, struct object *left, struct object *right) {
    long result;
    
    switch (opcode) {
        case OPCODE_ADD: 
            result = left->value.integer + right->value.integer;
        break;
        case OPCODE_SUBTRACT: 
            result = left->value.integer - right->value.integer;
        break;
        case OPCODE_MULTIPLY: 
            result = left->value.integer * right->value.integer;
        break;
        case OPCODE_DIVIDE: 
            result = left->value.integer / right->value.integer;
        break;
        default:
            return VM_ERR_INVALID_INT_OPERATOR;
        break;
    }

    vm_stack_push(vm, make_integer_object(result));
    return 0;
}

int vm_do_binary_operation(struct vm *vm, enum opcode opcode) {
    struct object *right = vm_stack_pop(vm);
    struct object *left = vm_stack_pop(vm);
    
    if (left->type == OBJ_INT && right->type == OBJ_INT) {
        return vm_do_binary_integer_operation(vm, opcode, left, right);
    }  

    return VM_ERR_INVALID_OP_TYPE;
}

int vm_do_integer_comparison(struct vm *vm, enum opcode opcode, struct object *left, struct object *right) {
    switch (opcode) {
         case OPCODE_EQUAL: 
            return vm_stack_push(vm, make_boolean_object(left->value.integer == right->value.integer));
        break;

        case OPCODE_NOT_EQUAL: 
            return vm_stack_push(vm, make_boolean_object(left->value.integer != right->value.integer));
        break;

        case OPCODE_GREATER_THAN: 
            return vm_stack_push(vm, make_boolean_object(left->value.integer > right->value.integer)); 
        break;

        case OPCODE_LESS_THAN:
            return vm_stack_push(vm, make_boolean_object(left->value.integer < right->value.integer)); 
        break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }
}


int vm_do_comparision(struct vm *vm, enum opcode opcode) {
    struct object *right = vm_stack_pop(vm);
    struct object *left = vm_stack_pop(vm);
    
    if (left->type == OBJ_INT && right->type == OBJ_INT) {
        return vm_do_integer_comparison(vm, opcode, left, right);
    }  

    switch (opcode) {
        case OPCODE_EQUAL: 
            return vm_stack_push(vm, make_boolean_object(left->value.boolean == right->value.boolean));
        break;

        case OPCODE_NOT_EQUAL: 
            return vm_stack_push(vm, make_boolean_object(left->value.boolean != right->value.boolean));
        break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }    
}

int vm_do_bang_operation(struct vm *vm) {
    struct object *obj = vm_stack_pop(vm);
    return vm_stack_push(vm, is_object_truthy(obj) ? object_false : object_true);
}

int vm_do_minus_operation(struct vm *vm) {
    struct object *obj = vm_stack_pop(vm);

    if (obj->type != OBJ_INT) {
        return VM_ERR_INVALID_OP_TYPE;
    }

    // TODO: Possibly re-use object here?
    return vm_stack_push(vm, make_integer_object(-obj->value.integer));
}

int vm_run(struct vm *vm) {
    size_t size = vm->instructions->size;
    unsigned char *bytes = vm->instructions->bytes;
    int err;

    for (int ip=0; ip < size; ip++) {
        enum opcode opcode = bytes[ip];
        switch (opcode) {
            case OPCODE_TRUE: 
                vm_stack_push(vm, object_true);
            break;

            case OPCODE_FALSE: 
                vm_stack_push(vm, object_false);
            break;

            case OPCODE_POP: {
                vm_stack_pop(vm);
            }
            break;
            
            case OPCODE_CONST: {
                int idx = read_bytes(bytes, ip+1, 2);
                ip += 2;
                vm_stack_push(vm, vm->constants->values[idx]);   
            }
            break;

            case OPCODE_ADD:
            case OPCODE_SUBTRACT:
            case OPCODE_MULTIPLY:
            case OPCODE_DIVIDE: {
                err = vm_do_binary_operation(vm, opcode);
                if (err) return err;
            }
            break;

            case OPCODE_BANG: 
                err = vm_do_bang_operation(vm);
                if (err) return err;
            break;

            case OPCODE_MINUS: 
                err = vm_do_minus_operation(vm);
                if (err) return err;
            break;

            case OPCODE_EQUAL:
            case OPCODE_NOT_EQUAL: 
            case OPCODE_GREATER_THAN: 
            case OPCODE_LESS_THAN: 
                err = vm_do_comparision(vm, opcode);
                if (err) return err;
            break;

            case OPCODE_JUMP: {
                int pos = read_bytes(bytes, ip+1, 2);
                ip = pos - 1;
            }
            break;

            case OPCODE_JUMP_NOT_TRUE: {
                int pos = read_bytes(bytes, ip+1, 2);
                ip += 2;

                struct object *condition = vm_stack_pop(vm);
                if (!is_object_truthy(condition)) {
                    ip = pos - 1;
                } 
            }
            break;

            case OPCODE_NULL: 
                err = vm_stack_push(vm, object_null);
                if (err) return err;
            break;

            case OPCODE_SET_GLOBAL: {
                int idx = read_bytes(bytes, ip+1, 2);
                ip += 2;
                vm->globals->values[idx] = vm_stack_pop(vm);
            }
            break;

            case OPCODE_GET_GLOBAL: {
                int idx = read_bytes(bytes, ip+1, 2);
                ip += 2;
                err = vm_stack_push(vm, vm->globals->values[idx]);
                if (err) return err;
            }
            break;

           
        }
    }

    return 0;
}

struct object *vm_stack_last_popped(struct vm *vm) {
    return vm->stack[vm->stack_pointer];
}