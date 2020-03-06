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

struct frame frame_new(struct object obj, unsigned int bp) {
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

    for (int i = 0; i < STACK_SIZE; i++) {
        vm->stack[i] = obj_null;
        vm->globals[i] = obj_null;
        vm->constants[i] = obj_null;
    }

    for (int i=0; i < bc->constants->size; i++) {
        vm->constants[i] = *bc->constants->values[i];
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

int vm_do_integer_comparison(struct vm *vm, enum opcode opcode, int left, int right) { 
    bool result;
    
    switch (opcode) {
         case OPCODE_EQUAL: 
            result = left == right;
            break;

        case OPCODE_NOT_EQUAL: 
            result = left != right;
            break;

        case OPCODE_GREATER_THAN: 
            result = left > right;
            break;

        case OPCODE_LESS_THAN:
            result = left < right;
            break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }

    vm_stack_push(vm, result ? obj_true : obj_false);
    return 0;
}


int vm_do_comparision(struct vm *vm, enum opcode opcode) {
    struct object right = vm_stack_pop(vm);
    struct object left = vm_stack_pop(vm);

    if (left.type != right.type) {
        return VM_ERR_INVALID_OP_TYPE;
    }
    
    if (left.type == OBJ_INT) {
        return vm_do_integer_comparison(vm, opcode, left.value.integer, right.value.integer);
    }  

    bool result;

    switch (opcode) {
        case OPCODE_EQUAL: 
            result = left.value.boolean == right.value.boolean;
        break;

        case OPCODE_NOT_EQUAL: 
            result = left.value.boolean != right.value.boolean;
        break;

        default: 
            return VM_ERR_INVALID_OP_TYPE;
        break;
    }    

    return vm_stack_push(vm, result ? obj_true : obj_false);
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

    /* values used in main loop */
    unsigned int ip;
    unsigned int ip_max;
    struct frame *active_frame;
    enum opcode opcode;
    uint8_t *bytes;

    /* tmp values used in switch cases */
    int err, idx, pos, num_args;

    static const void *dispatch_table[] = {
        &&GOTO_OPCODE_CONST,
        &&GOTO_OPCODE_POP,
        &&GOTO_OPCODE_ADD,
        &&GOTO_OPCODE_SUBTRACT,
        &&GOTO_OPCODE_MULTIPLY,
        &&GOTO_OPCODE_DIVIDE,
        &&GOTO_OPCODE_TRUE,
        &&GOTO_OPCODE_FALSE,
        &&GOTO_OPCODE_EQUAL,
        &&GOTO_OPCODE_NOT_EQUAL,
        &&GOTO_OPCODE_GREATER_THAN,
        &&GOTO_OPCODE_LESS_THAN,
        &&GOTO_OPCODE_MINUS,
        &&GOTO_OPCODE_BANG,
        &&GOTO_OPCODE_JUMP,
        &&GOTO_OPCODE_JUMP_NOT_TRUE,
        &&GOTO_OPCODE_NULL,
        &&GOTO_OPCODE_GET_GLOBAL,
        &&GOTO_OPCODE_SET_GLOBAL,
        &&GOTO_OPCODE_CALL,
        &&GOTO_OPCODE_RETURN_VALUE,
        &&GOTO_OPCODE_RETURN,
        &&GOTO_OPCODE_GET_LOCAL,
        &&GOTO_OPCODE_SET_LOCAL,
    };

    #define DISPATCH()                   \
        if (ip >= ip_max) { return 0; }  \
        opcode = bytes[ip];              \
        goto *dispatch_table[opcode];    \

    #ifdef DEBUG
    char str[512];
    printf("Running VM\nInstructions: %s\n", instruction_to_str(&vm->frames[0].fn.instructions));
    printf("Constants: \n");
    for (int i = 0; i < 4; i++) {
        str[0] = '\0';
        object_to_str(str, &vm->constants[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->constants[i].type), str);
    }
    #endif

    active_frame = &vm->frames[vm->frame_index];
    bytes = active_frame->fn.instructions.bytes;
    ip = active_frame->ip;
    ip_max = active_frame->fn.instructions.size;

   
     #ifdef DEBUG
     while (1) {    
        opcode = bytes[ip];
        printf("\nFrame: %2ld | IP: %3d/%ld | opcode: %16s | operand: ", vm->frame_index, ip, active_frame->fn.instructions.size - 1, opcode_to_str(bytes[ip]));
        struct definition def = lookup(opcode);
        if (def.operands > 0) {
            printf("%3d\n", read_bytes(bytes + ip + 1, def.operand_widths[0]));
        } else {
            printf("-\n");
        }

        printf("Globals: \n");
        for (int i = 0; i < 4; i++) {
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

        // intitial dispatch
        DISPATCH();

        GOTO_OPCODE_CONST:
            idx = read_uint16((bytes + ip + 1));
            ip += 3;
            vm_stack_push(vm, vm->constants[idx]);   
            DISPATCH();

        GOTO_OPCODE_POP:
            vm_stack_pop(vm);
            ip++;
            DISPATCH();
        
        GOTO_OPCODE_CALL: 
            num_args = read_uint8((bytes + ip + 1));
            struct object fn = vm->stack[vm->stack_pointer - 1 - num_args];
            struct frame f = frame_new(fn, vm->stack_pointer - num_args);
            active_frame->ip = ip + 1;
            vm_push_frame(vm, f);
            active_frame = &vm->frames[vm->frame_index];
            bytes = active_frame->fn.instructions.bytes;
            ip_max = active_frame->fn.instructions.size;
            ip = active_frame->ip;
            vm->stack_pointer = f.base_pointer + f.fn.num_locals;
            DISPATCH();

        GOTO_OPCODE_JUMP:
            pos = read_uint16((bytes + ip + 1));
            ip = pos;
            DISPATCH();

        GOTO_OPCODE_JUMP_NOT_TRUE: 
            pos = read_uint16((bytes + ip + 1));
            ip += 3;

            struct object condition = vm_stack_pop(vm);
            if (condition.type == OBJ_NULL || (condition.type == OBJ_BOOL && condition.value.boolean == false)) {
                ip = pos;
            } 
            DISPATCH();

        GOTO_OPCODE_SET_GLOBAL: 
            idx = read_uint16((bytes + ip + 1));
            ip += 3;
            vm->globals[idx] = vm_stack_pop(vm);
            DISPATCH();

        GOTO_OPCODE_GET_GLOBAL: 
            idx = read_uint16((bytes + ip + 1));
            ip += 3;
            vm_stack_push(vm, vm->globals[idx]);
            DISPATCH();

        GOTO_OPCODE_RETURN_VALUE: {
            struct object obj = vm_stack_pop(vm); // pop return value
            struct frame f = vm_pop_frame(vm);
            active_frame = &vm->frames[vm->frame_index];
            bytes = active_frame->fn.instructions.bytes;
            ip_max = active_frame->fn.instructions.size;
            ip = active_frame->ip;
            vm->stack_pointer = f.base_pointer - 1;
            vm_stack_push(vm, obj);
            ip++;
            DISPATCH();
        }
        
        GOTO_OPCODE_RETURN: {
            struct frame f = vm_pop_frame(vm);
            active_frame = &vm->frames[vm->frame_index];
            ip = active_frame->ip;
            ip_max = active_frame->fn.instructions.size;
            bytes = active_frame->fn.instructions.bytes;
            vm->stack_pointer = f.base_pointer - 1;
            vm_stack_push(vm, obj_null);
            DISPATCH();
        }

        GOTO_OPCODE_SET_LOCAL:
            idx = read_uint8((bytes + ip + 1));
            ip += 2;
            vm->stack[active_frame->base_pointer + idx] = vm_stack_pop(vm);
            DISPATCH();

        GOTO_OPCODE_GET_LOCAL: 
            idx = read_uint8((bytes + ip + 1));
            ip += 2;
            vm_stack_push(vm, vm->stack[active_frame->base_pointer + idx]);
            DISPATCH();

        GOTO_OPCODE_ADD:
        GOTO_OPCODE_SUBTRACT:
        GOTO_OPCODE_MULTIPLY:
        GOTO_OPCODE_DIVIDE: 
            err = vm_do_binary_operation(vm, opcode);
            if (err) return err;
            ip++;
            DISPATCH();

        GOTO_OPCODE_BANG: 
            err = vm_do_bang_operation(vm);
            if (err) return err;
            ip++;
            DISPATCH();

        GOTO_OPCODE_MINUS: 
            err = vm_do_minus_operation(vm);
            if (err) return err;
            ip++;
            DISPATCH();

        GOTO_OPCODE_EQUAL:
        GOTO_OPCODE_NOT_EQUAL: 
        GOTO_OPCODE_GREATER_THAN: 
        GOTO_OPCODE_LESS_THAN: 
            err = vm_do_comparision(vm, opcode);
            if (err) return err;
            ip++;
            DISPATCH();

        GOTO_OPCODE_TRUE: 
            vm_stack_push(vm, obj_true);
            ip++;
            DISPATCH();

        GOTO_OPCODE_FALSE: 
            vm_stack_push(vm, obj_false);
            ip++;
            DISPATCH();

        GOTO_OPCODE_NULL: 
            vm_stack_push(vm, obj_null);
            ip++;
            DISPATCH();

    #ifdef DEBUG 
    } // end while
    #endif 
    
    return 0;
}

struct object vm_stack_last_popped(struct vm *vm) {
    return vm->stack[vm->stack_pointer];
}
