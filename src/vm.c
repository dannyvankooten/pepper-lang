#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

struct vm *vm_new(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    assert(vm != NULL);
    vm->stack_pointer = 0;

    for (int i = 0; i < STACK_SIZE; i++) {
        vm->stack[i] = obj_null;
        vm->globals[i] = obj_null;
        vm->constants[i] = obj_null;
    }

    for (int i=0; i < bc->constants->size; i++) {
       vm->constants[i] = *bc->constants->values[i];
    }    

    // copy instructions here
    struct instruction *ins = malloc(sizeof (struct instruction));
    memcpy(ins, bc->instructions, sizeof(struct instruction));
    struct object *fn = make_compiled_function_object(ins, 0);
    vm->frames[0] = frame_new(*fn, 0);
    vm->frame_index = 0;
    free_object_shallow(fn);
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
    free(vm->frames[0].fn);
    free(vm);
}

struct frame frame_new(struct object obj, size_t bp) {
    assert(obj.type == OBJ_COMPILED_FUNCTION);
    struct frame f = {
        .ip = 0,
        .fn = obj.value.compiled_function,
        .base_pointer = bp,
    };

    return f;
}

#ifdef OPT_AGGRESSIVE
#define vm_pop_frame(vm) vm->frames[vm->frame_index--]
#define vm_push_frame(vm, f) vm->frames[++vm->frame_index] = f
#else
struct frame vm_pop_frame(struct vm *vm) {
    return vm->frames[vm->frame_index--];
}
void vm_push_frame(struct vm *vm, struct frame f) {
    assert(vm->frame_index + 1 < STACK_SIZE);
    vm->frames[++vm->frame_index] = f;
}
#endif

#define vm_stack_pop_ignore(vm) vm->stack_pointer--

#ifdef OPT_AGGRESSIVE
#define vm_stack_pop(vm) vm->stack[--vm->stack_pointer]
#define vm_stack_push(vm, obj) vm->stack[vm->stack_pointer++] = obj
#else 
struct object vm_stack_pop(struct vm *vm) {
    #ifndef UNSAFE
    if (vm->stack_pointer == 0) {
        return obj_null;
    }
    #endif

    return vm->stack[--vm->stack_pointer];
}

void vm_stack_push(struct vm *vm, struct object obj) {
    #ifndef UNSAFE
    if (vm->stack_pointer >= STACK_SIZE) {
        err(VM_ERR_STACK_OVERFLOW, "stack overflow");
        return;
    }
    #endif

    vm->stack[vm->stack_pointer++] = obj;
}
#endif

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

int vm_do_binary_string_operation(struct vm *vm, enum opcode opcode, char *left, char *right) {
    struct object obj = {
        .type = OBJ_STRING,
    };

    // TODO: Fix this... This allocation is not freed yet
    // We should probably assume ownership of all objects entering the VM
    // So that this becomes a lot easier
    obj.value.string = malloc(strlen(left) + strlen(right) + 1);
    assert(obj.value.string != NULL);
    strcpy(obj.value.string, left);
    strcat(obj.value.string, right);
    vm_stack_push(vm, obj);
    return 0;
}

int vm_do_binary_operation(struct vm *vm, enum opcode opcode) {
    struct object right = vm_stack_pop(vm);
    struct object left = vm_stack_pop(vm);
    assert(left.type == right.type);

    switch (left.type) {
        case OBJ_INT: return vm_do_binary_integer_operation(vm, opcode, left.value.integer, right.value.integer);
        case OBJ_STRING: return vm_do_binary_string_operation(vm, opcode, left.value.string, right.value.string);
        default: return VM_ERR_INVALID_OP_TYPE;
    }
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

    assert(left.type == right.type);
    
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

    vm_stack_push(vm, result ? obj_true : obj_false);
    return 0;
}

void vm_do_bang_operation(struct vm *vm) {
    struct object obj = vm_stack_pop(vm);
    vm_stack_push(vm, obj.type == OBJ_NULL || (obj.type == OBJ_BOOL && obj.value.boolean == false) ? obj_true : obj_false);
}

int vm_do_minus_operation(struct vm *vm) {
    struct object obj = vm_stack_pop(vm);
    assert(obj.type == OBJ_INT);
    obj.value.integer = -obj.value.integer;
    vm_stack_push(vm, obj);
    return 0;
}

int vm_run(struct vm *vm) {

    /* values used in main loop */
    size_t ip;
    size_t ip_max;
    struct frame *active_frame;
    enum opcode opcode;
    uint8_t *bytes;

    /* tmp values used in switch cases */
    int err, idx, pos, num_args;
    struct frame frame;

    /* 
    The following comment is taken from CPython's source: https://github.com/python/cpython/blob/master/Python/ceval.c#L775

    Computed GOTOs, or
       the-optimization-commonly-but-improperly-known-as-"threaded code"
   using gcc's labels-as-values extension
   (http://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html).

   The traditional bytecode evaluation loop uses a "switch" statement, which
   decent compilers will optimize as a single indirect branch instruction
   combined with a lookup table of jump addresses. However, since the
   indirect jump instruction is shared by all opcodes, the CPU will have a
   hard time making the right prediction for where to jump next (actually,
   it will be always wrong except in the uncommon case of a sequence of
   several identical opcodes).

   "Threaded code" in contrast, uses an explicit jump table and an explicit
   indirect jump instruction at the end of each opcode. Since the jump
   instruction is at a different address for each opcode, the CPU will make a
   separate prediction for each of these instructions, which is equivalent to
   predicting the second opcode of each opcode pair. These predictions have
   a much better chance to turn out valid, especially in small bytecode loops.

   A mispredicted branch on a modern CPU flushes the whole pipeline and
   can cost several CPU cycles (depending on the pipeline depth),
   and potentially many more instructions (depending on the pipeline width).
   A correctly predicted branch, however, is nearly free.

   At the time of this writing, the "threaded code" version is up to 15-20%
   faster than the normal "switch" version, depending on the compiler and the
   CPU architecture.

   We disable the optimization if DYNAMIC_EXECUTION_PROFILE is defined,
   because it would render the measurements invalid.

   NOTE: care must be taken that the compiler doesn't try to "optimize" the
   indirect jumps by sharing them between all opcodes. Such optimizations
   can be disabled on gcc by using the -fno-gcse flag (or possibly
   -fno-crossjumping).
*/
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
        opcode = bytes[ip];                \
        goto *dispatch_table[bytes[ip]];    \

    #ifdef DEBUG
    char str[512];
    printf("Running VM\nInstructions: %s\n", instruction_to_str(&vm->frames[0].fn->instructions));
    printf("Constants: \n");
    for (int i = 0; i < 4; i++) {
        str[0] = '\0';
        object_to_str(str, &vm->constants[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->constants[i].type), str);
    }
    #endif

    active_frame = &vm->frames[vm->frame_index];
    bytes = active_frame->fn->instructions.bytes;
    ip = active_frame->ip;
    ip_max = active_frame->fn->instructions.size;

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

        // loads a constant on the stack
        GOTO_OPCODE_CONST:
            idx = read_uint16((bytes + ip + 1));
            ip += 3;
            vm_stack_push(vm, vm->constants[idx]);   
            DISPATCH();

        // pop last value off the stack and discard it
        GOTO_OPCODE_POP:
            vm_stack_pop_ignore(vm);
            ip++;
            DISPATCH();
        
        GOTO_OPCODE_CALL: 
            num_args = read_uint8((bytes + ip + 1));
            //struct object fn = vm->stack[vm->stack_pointer - 1 - num_args];

            // grab next new frame from frame stack & re-use
            frame = vm->frames[vm->frame_index+1];
            frame.ip = 0;
            frame.fn = vm->stack[vm->stack_pointer - 1 - num_args].value.compiled_function;
            frame.base_pointer = vm->stack_pointer - num_args;
            active_frame->ip = ip + 1;
            vm_push_frame(vm, frame);
            active_frame = &vm->frames[vm->frame_index];
            bytes = frame.fn->instructions.bytes;
            ip_max = frame.fn->instructions.size;
            ip = active_frame->ip;
            vm->stack_pointer = frame.base_pointer + frame.fn->num_locals;
            DISPATCH();

        GOTO_OPCODE_JUMP:
            pos = read_uint16((bytes + ip + 1));
            ip = pos;
            DISPATCH();

        GOTO_OPCODE_JUMP_NOT_TRUE: {
            struct object condition = vm_stack_pop(vm);
            if (condition.type == OBJ_NULL || (condition.type == OBJ_BOOL && condition.value.boolean == false)) {
                pos = read_uint16((bytes + ip + 1));
                ip = pos;
            } else {
                ip += 3;
            }
            DISPATCH();
        }

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
            struct object obj = vm_stack_pop(vm); 
            struct frame f = vm_pop_frame(vm);
            active_frame = &vm->frames[vm->frame_index];
            bytes = active_frame->fn->instructions.bytes;
            ip_max = active_frame->fn->instructions.size;
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
            ip_max = active_frame->fn->instructions.size;
            bytes = active_frame->fn->instructions.bytes;
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
            #ifndef UNSAFE
            if (err) return err;
            #endif
            ip++;
            DISPATCH();

        GOTO_OPCODE_BANG: 
            vm_do_bang_operation(vm);
            ip++;
            DISPATCH();

        GOTO_OPCODE_MINUS: 
            err = vm_do_minus_operation(vm);
            #ifndef UNSAFE
            if (err) return err;
            #endif
            ip++;
            DISPATCH();

        GOTO_OPCODE_EQUAL:
        GOTO_OPCODE_NOT_EQUAL: 
        GOTO_OPCODE_GREATER_THAN: 
        GOTO_OPCODE_LESS_THAN: 
            err = vm_do_comparision(vm, opcode);
            #ifndef UNSAFE
            if (err) return err;
            #endif
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
