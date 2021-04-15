#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <util.h>

#include "object.h"
#include "opcode.h"
#include "vm.h"
#include "builtins.h"

#define vm_current_frame(vm) (&vm->frames[vm->frame_index]);
#define vm_stack_pop_ignore(vm) (vm->stack_pointer--)
#define vm_stack_pop(vm) (vm->stack[--vm->stack_pointer])
#define vm_stack_cur(vm) (vm->stack[vm->stack_pointer - 1])
#define vm_stack_push(vm, obj) (vm->stack[vm->stack_pointer++] = obj)

#ifndef DEBUG 
    #define DISPATCH() goto *dispatch_table[*frame->ip];        
#else 
    #define DISPATCH()                      \
        print_debug_info(vm);               \
        goto *dispatch_table[*frame->ip];      

static void 
print_debug_info(struct vm *vm) {
    char str[BUFSIZ] = {'\0'};
    struct frame *frame = vm_current_frame(vm);
    
    printf("\nFrame: %2d | IP: %3d/%d | opcode: %16s | operand: ", vm->frame_index, frame->ip, frame->fn->instructions.size - 1, opcode_to_str(*frame->ip));
    struct definition def = lookup(*frame->ip);
    if (def.operands > 0) {
        printf("%3d\n", read_bytes(frame->ip + 1, def.operand_widths[0]));
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
}
#endif 

struct vm *vm_new(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    assert(vm != NULL);
    vm->stack_pointer = 0;
    vm->frame_index = 0;

    for (int32_t i = 0; i < STACK_SIZE; i++) {
        vm->stack[i].type = OBJ_NULL;
    } 
    for (int32_t i=0; i < GLOBALS_SIZE; i++) {
        vm->globals[i].type = OBJ_NULL;
    }

    // copy over constants from compiled bytecode
    vm->constants = malloc(sizeof(struct object) * bc->constants->size);
    assert(vm->constants != NULL);
    vm->nconstants = bc->constants->size;
    for (int32_t i=0; i < bc->constants->size; i++) {
       vm->constants[i] = *bc->constants->values[i];
    }    

    // copy instruction as we are not adding this compiled function to a constant list
    // the bytes are freed through the bytecode object
    struct instruction *ins = malloc(sizeof (struct instruction));
    assert(ins != NULL);
    memcpy(ins, bc->instructions, sizeof(struct instruction));
    struct object *fn = make_compiled_function_object(ins, 0);
    vm->frames[0].ip = fn->value.compiled_function->instructions.bytes;
    vm->frames[0].fn = fn->value.compiled_function;
    vm->frames[0].base_pointer = 0;
    free_object_shallow(fn);
    return vm;
}

struct vm *vm_new_with_globals(struct bytecode *bc, struct object globals[GLOBALS_SIZE]) {
    struct vm *vm = vm_new(bc);

    for (int32_t i=0; i < GLOBALS_SIZE; i++) {
        vm->globals[i] = globals[i];
    }
    return vm;
}

void vm_free(struct vm *vm) {
    /* free initial compiled function since it's not on the constants list */
    free(vm->frames[0].fn);

    /* free vm itself */
    free(vm->constants);
    free(vm);
}


static void 
vm_do_binary_integer_operation(const struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {    
    switch (opcode) {
        case OPCODE_ADD: 
            left->value.integer += right->value.integer;
        break;
        case OPCODE_SUBTRACT: 
            left->value.integer -= right->value.integer;
        break;
        case OPCODE_MULTIPLY: 
            left->value.integer *= right->value.integer;
        break;
        case OPCODE_DIVIDE: 
            left->value.integer /= right->value.integer;
        break;
        default:
            err(VM_ERR_INVALID_INT_OPERATOR, "Invalid operator for integer operation.");
        break;
    }
}

static void 
vm_do_binary_string_operation(const struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
    // TODO: Fix this... This allocation is not freed yet
    // We should probably assume ownership of all objects entering the VM
    // So that this becomes a lot easier
    char *ptr = realloc(left->value.string, strlen(left->value.string) + strlen(right->value.string) + 1);
    assert(ptr != NULL);
    strcat(ptr, right->value.string);
    left->value.string = ptr;
}

static void 
vm_do_binary_operation(struct vm* restrict vm, const enum opcode opcode) {
    const struct object* right = &vm_stack_pop(vm);
    struct object* left = &vm_stack_cur(vm);
    assert(left->type == right->type);

    switch (left->type) {
        case OBJ_INT: 
            return vm_do_binary_integer_operation(vm, opcode, left, right); 
        break;
        case OBJ_STRING: 
            return vm_do_binary_string_operation(vm, opcode, left, right); 
        break;
        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid type for binary operation.");
        break;
    }
}

static void 
vm_do_integer_comparison(const struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {   
    switch (opcode) {
         case OPCODE_EQUAL: 
            left->value.boolean = left->value.integer == right->value.integer;
            break;

        case OPCODE_NOT_EQUAL: 
            left->value.boolean = left->value.integer != right->value.integer;
            break;

        case OPCODE_GREATER_THAN: 
            left->value.boolean = left->value.integer > right->value.integer;
            break;

        case OPCODE_LESS_THAN:
            left->value.boolean = left->value.integer < right->value.integer;
            break;

        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid operator for integer comparison");
        break;
    }

    left->type = OBJ_BOOL;
}

static void
vm_do_bool_comparison(const struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
    switch (opcode) {
        case OPCODE_EQUAL: 
            left->value.boolean = left->value.boolean == right->value.boolean;
        break;

        case OPCODE_NOT_EQUAL: 
            left->value.boolean = left->value.boolean != right->value.boolean;
        break;

        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid operator for boolean comparison.");
        break;
    }    
}

static void 
vm_do_comparision(struct vm* restrict vm, const enum opcode opcode) {
    const struct object* right = &vm_stack_pop(vm);
    struct object* left = &vm_stack_cur(vm);
    assert(left->type == right->type);

    switch (left->type) {
        case OBJ_INT:
            return vm_do_integer_comparison(vm, opcode, left, right);
        break;

        case OBJ_BOOL:
            return vm_do_bool_comparison(vm, opcode, left, right);
        break;

        default:
            err(VM_ERR_INVALID_OP_TYPE, "Invalid type for comparison.");
        break;
    }   
}

static void
vm_do_bang_operation(struct vm * restrict vm) {
    // modify item in place by leaving it on the stack
    struct object* obj = &vm_stack_cur(vm);
    obj->value.boolean = obj->type == OBJ_NULL || (obj->type == OBJ_BOOL && obj->value.boolean == false) || (obj->type == OBJ_INT && obj->value.integer <= 0);
    obj->type = OBJ_BOOL;
}

static void  
vm_do_minus_operation(struct vm * restrict vm) {
    // modify item in place by leaving it on the stack
    vm_stack_cur(vm).value.integer *= -1;
}

/* handle call to built-in function */
static void 
vm_do_call_builtin(struct vm *vm, struct object *(*builtin)(struct object_list *), const uint8_t num_args) {
    // create object list with arguments
    struct object_list *args = make_object_list(num_args);
    for (uint32_t i = vm->stack_pointer - num_args; i < vm->stack_pointer; i++) {
        args->values[args->size++] = &vm->stack[i];
    }   

    struct object *result = builtin(args);
    vm->stack_pointer = vm->stack_pointer - num_args - 1;
    vm_stack_push(vm, *result);
    vm->frames[vm->frame_index].ip++;

    free_object_shallow(result);
    free_object_list_shallow(args);
}

/* handle call to user-defined function */
static void 
vm_do_call_function(struct vm *vm, struct compiled_function *f, const uint8_t num_args) {
    /* TODO: Validate number of arguments */

     // Push new frame (from pre-allocated list)
    struct frame* frame = &vm->frames[++vm->frame_index];
    frame->ip = f->instructions.bytes;
    frame->fn = f;
    frame->base_pointer = vm->stack_pointer - num_args;
    vm->stack_pointer = frame->base_pointer + f->num_locals; 
}

static void
vm_do_call(struct vm* restrict vm, const uint8_t num_args) {
    const struct object callee = vm->stack[vm->stack_pointer - 1 - num_args];
    switch (callee.type) {
        case OBJ_COMPILED_FUNCTION:
            return vm_do_call_function(vm, callee.value.compiled_function, num_args);
        break;

        case OBJ_BUILTIN:
            return vm_do_call_builtin(vm, callee.value.builtin, num_args);
        break;

        default:
            err(VM_ERR_INVALID_FUNCTION_CALL, "Invalid function call.");
        break;
    }
}

enum result 
vm_run(struct vm* restrict vm) {
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
    const void *dispatch_table[] = {
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
        &&GOTO_OPCODE_GET_BUILTIN,
        &&GOTO_OPCODE_HALT,
    };
    struct frame *frame = vm_current_frame(vm);

    #ifdef DEBUG
    char str[512];
    char *instruction_str = instruction_to_str(&frame->fn->instructions);
    printf("Running VM\nInstructions: %s\n", instruction_str);
    free(instruction_str);
    printf("Constants: \n");
    for (int i = 0; i < vm->nconstants; i++) {
        str[0] = '\0';
        object_to_str(str, &vm->constants[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->constants[i].type), str);
    }

    print_debug_info(vm);
    #endif 


    // intitial dispatch
    DISPATCH();

    // pushes a constant on the stack
    GOTO_OPCODE_CONST: {
        uint16_t idx = read_uint16((frame->ip + 1));
        frame->ip += 3;
        vm_stack_push(vm, vm->constants[idx]);   
        DISPATCH();
    }

    // pop last value off the stack and discard it
    GOTO_OPCODE_POP: {
        vm_stack_pop_ignore(vm);
        frame->ip++;
        DISPATCH();
    }

    // call a (user-defined or built-in) function
    GOTO_OPCODE_CALL: {
        uint8_t num_args = read_uint8((++frame->ip));
        vm_do_call(vm, num_args);
        frame = &vm->frames[vm->frame_index];
        DISPATCH();
    }

    GOTO_OPCODE_JUMP: {
        uint16_t pos = read_uint16((frame->ip + 1));
        frame->ip = frame->fn->instructions.bytes + pos;
        DISPATCH();
    }

    GOTO_OPCODE_JUMP_NOT_TRUE: {
        // struct object condition = vm_stack_pop(vm);
        struct object *condition = &vm_stack_pop(vm);
        if (condition->type == OBJ_NULL || (condition->type == OBJ_BOOL && condition->value.boolean == false)) {
            uint16_t pos = read_uint16((frame->ip + 1));
            frame->ip = frame->fn->instructions.bytes + pos;
        } else {
            frame->ip += 3;
        }
        DISPATCH();
    }

    GOTO_OPCODE_SET_GLOBAL: {
        uint16_t idx = read_uint16((frame->ip + 1));
        frame->ip += 3;
        vm->globals[idx] = vm_stack_pop(vm);
        DISPATCH();
    }

    GOTO_OPCODE_GET_GLOBAL: {
        uint16_t idx = read_uint16((frame->ip + 1));
        frame->ip += 3;
        vm_stack_push(vm, vm->globals[idx]);
        DISPATCH();
    }

    GOTO_OPCODE_RETURN_VALUE: {
        struct object obj = vm_stack_pop(vm); 
        vm->stack_pointer = frame->base_pointer - 1;
        frame = &vm->frames[--vm->frame_index];
        vm_stack_push(vm, obj);
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_RETURN: {
        vm->stack_pointer = frame->base_pointer - 1;
        frame = &vm->frames[--vm->frame_index];
        vm->stack[vm->stack_pointer++].type = OBJ_NULL;
        frame->ip++; 
        DISPATCH();
    }

    GOTO_OPCODE_SET_LOCAL: {
        uint8_t idx = read_uint8((frame->ip + 1));
        frame->ip += 2;
        vm->stack[frame->base_pointer + idx] = vm_stack_pop(vm);
        DISPATCH();
    }

    GOTO_OPCODE_GET_LOCAL: {
        uint8_t idx = read_uint8((frame->ip + 1));
        frame->ip += 2;
        vm_stack_push(vm, vm->stack[frame->base_pointer + idx]);
        DISPATCH();
    }

    GOTO_OPCODE_ADD:
    GOTO_OPCODE_SUBTRACT:
    GOTO_OPCODE_MULTIPLY:
    GOTO_OPCODE_DIVIDE: {
        vm_do_binary_operation(vm, *frame->ip++);
        DISPATCH();
    }

    GOTO_OPCODE_BANG: {
        vm_do_bang_operation(vm);
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_MINUS: {
        vm_do_minus_operation(vm);
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_EQUAL:
    GOTO_OPCODE_NOT_EQUAL: 
    GOTO_OPCODE_GREATER_THAN: 
    GOTO_OPCODE_LESS_THAN: {
        vm_do_comparision(vm, *frame->ip++);
        DISPATCH();
    }

    GOTO_OPCODE_TRUE: {
        vm->stack[vm->stack_pointer].type = OBJ_BOOL;
        vm->stack[vm->stack_pointer].value.boolean = true;
        vm->stack_pointer++;
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_FALSE: {
        vm->stack[vm->stack_pointer].type = OBJ_BOOL;
        vm->stack[vm->stack_pointer].value.boolean = false;
        vm->stack_pointer++;
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_NULL: {
        vm->stack[vm->stack_pointer++].type = OBJ_NULL;
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_GET_BUILTIN: {
        uint8_t idx = read_uint8((frame->ip + 1));
        frame->ip += 2;
        vm_stack_push(vm, *get_builtin_by_index(idx));
        DISPATCH();
    }

    GOTO_OPCODE_HALT: ;

    return VM_SUCCESS;
}

struct object vm_stack_last_popped(struct vm *vm) {
    return vm->stack[vm->stack_pointer];
}
