#include <stdlib.h>
#include <assert.h>
#include <err.h>
#include "vm.h"

#ifdef DEBUG
#include <stdio.h>
#endif 

#define VM_ERR_INVALID_OP_TYPE 1
#define VM_ERR_INVALID_INT_OPERATOR 2
#define VM_ERR_OUT_OF_BOUNDS 3
#define VM_ERR_STACK_OVERFLOW 4

const struct object obj_null = {
    .type = OBJ_NULL,
};
const struct object obj_true = {
    .type = OBJ_BOOL,
    .value = { .boolean = true },
};
const struct object obj_false = {
    .type = OBJ_BOOL,
    .value = { .boolean = false },
};

struct frame frame_new(struct object obj, size_t bp) {
    assert(obj.type == OBJ_COMPILED_FUNCTION);
    struct frame f = {
        .ip = 0,
        .fn = *obj.value.compiled_function,
        .base_pointer = bp,
    };

    return f;
}

struct frame vm_current_frame(struct vm *vm) {
    return vm->frames[vm->frame_index];
}

struct frame vm_pop_frame(struct vm *vm) {
    return vm->frames[vm->frame_index--];
}

void vm_push_frame(struct vm *vm, struct frame f) {
    if ((vm->frame_index + 1) >= STACK_SIZE) {
        err(VM_ERR_STACK_OVERFLOW, "frame overflow");
    }
    vm->frames[++vm->frame_index] = f;
}


struct vm *vm_new(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    vm->stack_pointer = 0;

    for (int i=0; i < bc->constants->size; i++) {
        vm->constants[i] = *bc->constants->values[i];
    }

    for (int i = 0; i < STACK_SIZE; i++) {
        vm->stack[i] = obj_null;
    }

    struct object *fn = make_compiled_function_object(bc->instructions, 0);
    vm->frames[0] = frame_new(*fn, 0);
    vm->frame_index = 0;
    free_object(fn);
    return vm;
}

struct vm *vm_new_with_globals(struct bytecode *bc, struct object globals[STACK_SIZE]) {
    struct vm *vm = vm_new(bc);

    for (int i=0; i < STACK_SIZE; i++) {
        vm->globals[i] = globals[i];
    }
    return vm;
}

void vm_free(struct vm *vm) {
    free(vm);
}

struct object vm_stack_pop(struct vm *vm) {
    if (vm->stack_pointer == 0) {
        return obj_null;
    }

    struct object obj = vm->stack[--vm->stack_pointer];
    return obj;
}

int vm_stack_push(struct vm *vm, struct object obj) {
    if (vm->stack_pointer >= STACK_SIZE) {
        err(VM_ERR_STACK_OVERFLOW, "stack overflow");
        return VM_ERR_STACK_OVERFLOW;
    }

    vm->stack[vm->stack_pointer++] = obj;
    return 0;
}

int vm_do_binary_integer_operation(struct vm *vm, enum opcode opcode, int left, int right) {
    long result;
    
    switch (opcode) {
        case OPCODE_ADD: 
            result = left + right;
        break;
        case OPCODE_SUBTRACT: 
            result = left - right;
        break;
        case OPCODE_MULTIPLY: 
            result = left * right;
        break;
        case OPCODE_DIVIDE: 
            result = left / right;
        break;
        default:
            return VM_ERR_INVALID_INT_OPERATOR;
        break;
    }

    struct object obj = {
        .type = OBJ_INT,
        .value = { .integer = result }
    };
    // left.value.integer = result;
    vm_stack_push(vm, obj);
    return 0;
}

int vm_do_binary_operation(struct vm *vm, enum opcode opcode) {
    struct object right = vm_stack_pop(vm);
    struct object left = vm_stack_pop(vm);
    
    if (left.type == OBJ_INT && right.type == OBJ_INT) {
        return vm_do_binary_integer_operation(vm, opcode, left.value.integer, right.value.integer);
    }  

    return VM_ERR_INVALID_OP_TYPE;
}

int vm_do_integer_comparison(struct vm *vm, enum opcode opcode, struct object left, struct object right) {
    switch (opcode) {
         case OPCODE_EQUAL: 
            return vm_stack_push(vm, left.value.integer == right.value.integer ? obj_true : obj_false);
        break;

        case OPCODE_NOT_EQUAL: 
            return vm_stack_push(vm, left.value.integer != right.value.integer ? obj_true : obj_false);
        break;

        case OPCODE_GREATER_THAN: 
            return vm_stack_push(vm, left.value.integer > right.value.integer ? obj_true : obj_false); 
        break;

        case OPCODE_LESS_THAN:
            return vm_stack_push(vm, left.value.integer < right.value.integer ? obj_true : obj_false); 
        break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }
}


int vm_do_comparision(struct vm *vm, enum opcode opcode) {
    struct object right = vm_stack_pop(vm);
    struct object left = vm_stack_pop(vm);
    
    if (left.type == OBJ_INT && right.type == OBJ_INT) {
        return vm_do_integer_comparison(vm, opcode, left, right);
    }  

    switch (opcode) {
        case OPCODE_EQUAL: 
            return vm_stack_push(vm, left.value.boolean == right.value.boolean ? obj_true : obj_false);
        break;

        case OPCODE_NOT_EQUAL: 
            return vm_stack_push(vm, left.value.boolean != right.value.boolean ? obj_true : obj_false);
        break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }    
}

int vm_do_bang_operation(struct vm *vm) {
    struct object obj = vm_stack_pop(vm);
    return vm_stack_push(vm, obj.type == OBJ_NULL || (obj.type == OBJ_BOOL && obj.value.boolean == false) ? obj_true : obj_false);
}

int vm_do_minus_operation(struct vm *vm) {
    struct object obj = vm_stack_pop(vm);

    if (obj.type != OBJ_INT) {
        return VM_ERR_INVALID_OP_TYPE;
    }

    obj.value.integer = -obj.value.integer;
    return vm_stack_push(vm, obj);
}

int vm_run(struct vm *vm) {
    int err;
    struct frame frame;
    int ip;
    enum opcode opcode;
    unsigned char *bytes;

    #ifdef DEBUG
    char str[256];
    printf("Running VM\nInstructions: %s\n", instruction_to_str(&vm->frames[0].fn.instructions));
    printf("Constants: \n");
    for (int i = 0; i < 6; i++) {
        str[0] = '\0';
        object_to_str(str, &vm->constants.values[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->constants[i].type), str);
    }
    #endif
   
    while (1) {
        frame = vm->frames[vm->frame_index];
        ip = frame.ip;
        if (ip >= frame.fn.instructions.size) {
            #ifdef DEBUG
            printf("Stopping VM\n");
            #endif
            break;
        }
        bytes = frame.fn.instructions.bytes;
        opcode = bytes[ip];

        #ifdef DEBUG
        printf("\nFrame: %2ld | IP: %3d/%ld | opcode: %16s | operand: ", vm->frame_index, ip, ins.size - 1, opcode_to_str(bytes[ip]));
        struct definition def = lookup(opcode);
        if (def.operands > 0) {
            printf("%3d\n", read_bytes(bytes, ip+1, def.operand_widths[0]));
        } else {
            printf("-\n");
        }

        printf("Globals: \n");
        for (int i = 0; i < 6; i++) {
            str[0] = '\0';
            object_to_str(str, &vm->globals[i]);
            printf("  %3d: %s = %s\n", i, object_type_to_str(vm->globals[i].type), str);
        }

        printf("Stack: \n");
        for (int i=0; i < vm->stack_pointer; i++) {
            str[0] = '\0';
            object_to_str(str, &vm->stack[i]);
            printf("  %3d: %s = %s\n", i, object_type_to_str(vm->stack[i].type), str);
        }
        #endif 
                        
        switch (opcode) {
            case OPCODE_POP: {
                vm_stack_pop(vm);
            }
            break;
            
            case OPCODE_CONST: {
                int idx = read_bytes(bytes, ip+1, 2);
                vm->frames[vm->frame_index].ip += 2;
                vm_stack_push(vm, vm->constants[idx]);   
            }
            break;

            case OPCODE_JUMP: {
                int pos = read_bytes(bytes, ip+1, 2);
                vm->frames[vm->frame_index].ip = pos - 1;
            }
            break;

            case OPCODE_JUMP_NOT_TRUE: {
                int pos = read_bytes(bytes, ip + 1, 2);
                vm->frames[vm->frame_index].ip += 2;

                struct object condition = vm_stack_pop(vm);
                if (condition.type == OBJ_NULL || (condition.type == OBJ_BOOL && condition.value.boolean == false)) {
                    vm->frames[vm->frame_index].ip = pos - 1;
                } 
            }
            break;

             case OPCODE_SET_GLOBAL: {
                int idx = read_bytes(bytes, ip + 1, 2);
                vm->frames[vm->frame_index].ip += 2;
                struct object obj = vm_stack_pop(vm);
                vm->globals[idx] = obj;
            }
            break;

            case OPCODE_GET_GLOBAL: {
                int idx = read_bytes(bytes, ip + 1, 2);
                vm->frames[vm->frame_index].ip += 2;
                struct object obj = vm->globals[idx];
                vm_stack_push(vm, obj);
            }
            break;

            case OPCODE_CALL: {
                int num_args = read_bytes(bytes, ip + 1, 1);
                vm->frames[vm->frame_index].ip += 1;
                struct object fn = vm->stack[vm->stack_pointer - 1 - num_args];
                struct frame f = frame_new(fn, vm->stack_pointer - num_args);
                vm_push_frame(vm, f);
                vm->stack_pointer = f.base_pointer + f.fn.num_locals;
                // to skip incrementing the instruction pointer at the end of this loop
                continue;
            }
            break;

            case OPCODE_RETURN_VALUE: {
                struct object obj = vm_stack_pop(vm); // pop return value
                struct frame f = vm_pop_frame(vm);
                //vm_stack_pop(vm); // pop the function

                vm->stack_pointer = f.base_pointer - 1;

                // push return value back on stack
                vm_stack_push(vm, obj);
            }
            break;


            case OPCODE_RETURN: {
                struct frame f = vm_pop_frame(vm);
                vm->stack_pointer = f.base_pointer - 1;
                vm_stack_push(vm, obj_null);
            }
            break;

            case OPCODE_SET_LOCAL: {
                int idx = read_bytes(bytes, ip+1, 1);
                vm->frames[vm->frame_index].ip += 1;
                vm->stack[frame.base_pointer + idx] = vm_stack_pop(vm);
            }
            break;

            case OPCODE_GET_LOCAL: {
                int idx = read_bytes(bytes, ip+1, 1);
                vm->frames[vm->frame_index].ip += 1;
                vm_stack_push(vm, vm->stack[frame.base_pointer + idx]);
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

            case OPCODE_TRUE: 
                vm_stack_push(vm, obj_true);
            break;

            case OPCODE_FALSE: 
                vm_stack_push(vm, obj_false);
            break;

            case OPCODE_EQUAL:
            case OPCODE_NOT_EQUAL: 
            case OPCODE_GREATER_THAN: 
            case OPCODE_LESS_THAN: 
                err = vm_do_comparision(vm, opcode);
                if (err) return err;
            break;

            case OPCODE_NULL: 
                vm_stack_push(vm, obj_null);
            break;

        }

        vm->frames[vm->frame_index].ip++;
    }

    return 0;
}

struct object vm_stack_last_popped(struct vm *vm) {
    return vm->stack[vm->stack_pointer];
}
