#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "util.h"
#include "object.h"
#include "opcode.h"
#include "vm.h"
#include "builtins.h"
#include "gc.h"

#define vm_current_frame(vm) (vm->frames[vm->frame_index])
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
    struct frame *frame = &vm_current_frame(vm);

    int ip_now = frame->ip - frame->fn->instructions.bytes;
    int ip_end = frame->fn->instructions.size - 1;
    printf("\n\nFrame: %2d | IP: %3d/%d | opcode: %12s | operand: ", vm->frame_index, ip_now, ip_end, opcode_to_str(*frame->ip));
    struct definition def = lookup(*frame->ip);
    if (def.operands > 0) {
        if (def.operand_widths[0] > 1) {
            printf("%3d\n", read_uint16(frame->ip + 1));
        } else {
            printf("%3d\n", read_uint8(frame->ip + 1));
        }
        
    } else {
        printf("-\n");
    }

    printf("Constants: \n");
    for (unsigned i = 0; i < vm->nconstants; i++) {
        str[0] = '\0';
        object_to_str(str, vm->constants[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->constants[i].type), str);
    }

    printf("Globals: \n");
    for (unsigned i = 0; i < GLOBALS_SIZE; i++) {
        if (vm->globals[i].type == OBJ_NULL) {
            break;
        }
        str[0] = '\0';
        object_to_str(str, vm->globals[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->globals[i].type), str);
    }

    printf("Stack: \n");
    for (unsigned i=0; i < vm->stack_pointer; i++) {
        str[0] = '\0';
        object_to_str(str, vm->stack[i]);
        printf("  %3d: %s = %s\n", i, object_type_to_str(vm->stack[i].type), str);
    }
}
#endif 

static struct object_list *_builtin_args_list;

struct vm *vm_new(struct bytecode *bc) {
    struct vm *vm = malloc(sizeof *vm);
    assert(vm != NULL);
    vm->stack_pointer = 0;
    vm->frame_index = 0;

    // initialize globals as null objects for print_debug_info()
    for (unsigned i=0; i < GLOBALS_SIZE; i++) {
        vm->globals[i].type = OBJ_NULL;
    }

    // copy over constants from compiled bytecode
    vm->nconstants = 0;
    for (unsigned i=0; i < bc->constants->size; i++) {
       vm->constants[vm->nconstants++] = bc->constants->values[i];
    }    

    _builtin_args_list = make_object_list(32);

    // initialize heap
    vm->heap = make_object_list(256);

    struct object fn_obj = make_compiled_function_object(bc->instructions, 0);
    struct compiled_function* fn = fn_obj.value.fn_compiled;
    vm->frames[0].ip = fn->instructions.bytes;
    vm->frames[0].fn = fn;
    vm->frames[0].base_pointer = 0;
    return vm;
}

struct vm *vm_new_with_globals(struct bytecode *bc, struct object globals[GLOBALS_SIZE]) {
    struct vm *vm = vm_new(bc);

    for (unsigned i=0; i < GLOBALS_SIZE; i++) {
        vm->globals[i] = globals[i];
    }
    return vm;
}

void vm_free(struct vm *vm) {
    /* free initial compiled function since it's not on the constants list */
    free(vm->frames[0].fn);

    // free args list for builtin functions
    free_object_list(_builtin_args_list);

    // free all objects on heap
    free_object_list(vm->heap);

    /* free vm itself */
    free(vm);
}


static void 
vm_do_binary_integer_operation(struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {    
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
            if (right->value.integer == 0) {
                vm_stack_cur(vm) = make_error_object("Division by zero");
                gc_add(vm, vm_stack_cur(vm));
                return;
            }

            left->value.integer /= right->value.integer;
        break;
        case OPCODE_MODULO:
            if (right->value.integer == 0) {
                vm_stack_cur(vm) = make_error_object("Division by zero");
                gc_add(vm, vm_stack_cur(vm));
                return;
            }

            left->value.integer %= right->value.integer;
        break;
        default:
            err(VM_ERR_INVALID_OPERATOR, "Invalid operator %s for integer operation.", opcode_to_str(opcode));
        break;
    }
}

static void 
vm_do_binary_string_operation(struct vm* restrict vm, enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
    switch (opcode) {
        case OPCODE_ADD: {            
            struct object o = concat_string_objects(left->value.string, right->value.string);
            vm_stack_cur(vm) = o;
            gc_add(vm, o);   
        }
        break;

        default:
            err(VM_ERR_INVALID_OPERATOR, "Invalid operator %s for integer operation.", opcode_to_str(opcode));
        break;
    }
}

static void 
vm_do_binary_boolean_operation(__attribute__((unused)) struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
    switch (opcode) {
        case OPCODE_AND: 
            left->value.boolean = left->value.boolean && right->value.boolean;
        break;
        case OPCODE_OR: 
            left->value.boolean = left->value.boolean || right->value.boolean;
        break;
        default:
            err(VM_ERR_INVALID_OPERATOR, "Invalid operator for boolean operation.");
        break;
    }
}

static void 
vm_do_binary_operation(struct vm* restrict vm, const enum opcode opcode) {
    const struct object* right = &vm_stack_pop(vm);
    struct object* left = &vm_stack_cur(vm);
    assert(left->type == right->type);

    switch (left->type) {
        case OBJ_INT: 
            vm_do_binary_integer_operation(vm, opcode, left, right); 
        break;
        case OBJ_STRING: 
            vm_do_binary_string_operation(vm, opcode, left, right); 
        break;
        case OBJ_BOOL:
            vm_do_binary_boolean_operation(vm, opcode, left, right);
        break;
        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid type %s for binary operation.", object_type_to_str(left->type));
        break;
    }
}

static void 
vm_do_integer_comparison(__attribute__((unused)) struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
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

        case OPCODE_GREATER_THAN_OR_EQUALS: 
            left->value.boolean = left->value.integer >= right->value.integer;
            break;

        case OPCODE_LESS_THAN:
            left->value.boolean = left->value.integer < right->value.integer;
            break;

        case OPCODE_LESS_THAN_OR_EQUALS:
            left->value.boolean = left->value.integer <= right->value.integer;
            break;

        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid operator for integer comparison");
        break;
    }

    left->type = OBJ_BOOL;
}

static void
vm_do_bool_comparison(__attribute__((unused)) struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
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
vm_do_string_comparison(__attribute__((unused)) struct vm* restrict vm, const enum opcode opcode, struct object* restrict left, const struct object* restrict right) {
    left->type = OBJ_BOOL;
    switch (opcode) {
        case OPCODE_EQUAL: 
            left->value.boolean = strcmp(left->value.string->value, right->value.string->value) == 0;
        break;

        case OPCODE_NOT_EQUAL: 
            left->value.boolean = strcmp(left->value.string->value, right->value.string->value) != 0;
        break;

        default: 
            err(VM_ERR_INVALID_OP_TYPE, "Invalid operator %s for string comparison.", opcode_to_str(opcode));
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
            vm_do_integer_comparison(vm, opcode, left, right);
        break;

        case OBJ_BOOL:
            vm_do_bool_comparison(vm, opcode, left, right);
        break;

        case OBJ_STRING:
            vm_do_string_comparison(vm, opcode, left, right);
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
vm_do_minus_operation(struct vm* restrict vm) {
    // modify item in place by leaving it on the stack
    vm_stack_cur(vm).value.integer *= -1;
}

/* handle call to built-in function */
static void 
vm_do_call_builtin(struct vm* restrict vm, struct object (*builtin)(const struct object_list *), const uint8_t num_args) {
    struct object_list *args = _builtin_args_list;
    
    for (uint32_t i = vm->stack_pointer - num_args; i < vm->stack_pointer; i++) {
        args->values[args->size++] = vm->stack[i];
    }  

    struct object obj = builtin(args);
    vm->stack_pointer = vm->stack_pointer - num_args - 1;
    vm_stack_push(vm, obj);
    vm_current_frame(vm).ip++;
    
    // reset args for next use
    args->size = 0;

    // register result object in heap for GC
    gc_add(vm, obj);
}

/* handle call to user-defined function */
static void 
vm_do_call_function(struct vm* restrict vm, struct compiled_function* restrict fn, uint8_t num_args) {
    struct frame* frame = &vm->frames[++vm->frame_index];
    frame->ip = fn->instructions.bytes;
    frame->fn = fn;
    frame->base_pointer = vm->stack_pointer - num_args;
    vm->stack_pointer = frame->base_pointer + fn->num_locals; 
}

static void
vm_do_call(struct vm* restrict vm, uint8_t num_args) {
    const struct object callee = vm->stack[vm->stack_pointer - 1 - num_args];
    switch (callee.type) {
        case OBJ_COMPILED_FUNCTION:
            vm_do_call_function(vm, callee.value.fn_compiled, num_args);
        break;

        case OBJ_BUILTIN:
            vm_do_call_builtin(vm, callee.value.fn_builtin, num_args);
        break;

        default:
            err(VM_ERR_INVALID_FUNCTION_CALL, "Invalid function call.");
        break;
    }
}

static struct object 
vm_build_array(struct vm* restrict vm, uint16_t start_index, uint16_t end_index) {
    struct object_list* list = make_object_list(end_index - start_index);
    for (int32_t i = start_index; i < end_index; i++) {
        list->values[list->size++] = copy_object(&vm->stack[i]);
    }
    return make_array_object(list);
}

static struct object build_slice_from_array(struct object_list* source, int32_t start, int32_t end)
{

    if (start < 0) {
        start = source->size + start;
    }
    if (end <= 0) {
        end = source->size + end;
    }
    if (end < start) {
        end = start;
    }
    struct object_list* list = make_object_list(end - start);
    for (int i=start; i < end && i < (int) source->size; i++) {
        list->values[list->size++] = source->values[i];
    }
    return make_array_object(list);
}

static struct object build_slice_from_string(struct string* source, int32_t start, int32_t end)
{
    if (start < 0) {
        start = source->length + start;
    }
    if (end <= 0) {
        end = source->length + end;
    }
    if (end < start) {
        end = start;
    }
    struct object obj = make_string_object_with_length("", end - start);
    struct string* str = obj.value.string;
    str->length = 0;
    for (int i=start; i < end && i < (int) source->length; i++) {
        str->value[str->length++] = source->value[i];
    }
    str->value[str->length] = '\0';
    return obj;
}

static struct object 
build_slice(struct object left, struct object obj_start, struct object obj_end) 
{
    if ((obj_end.type != OBJ_NULL && obj_end.type != OBJ_INT) || (obj_start.type != OBJ_NULL && obj_start.type != OBJ_INT)) {
        return make_error_object("Slice indices must be integers.");
    }
    int32_t start = obj_start.type == OBJ_NULL ? 0 : obj_start.value.integer;
    int32_t end = obj_end.type == OBJ_NULL ? 0 : obj_end.value.integer;

    switch (left.type) {
        case OBJ_ARRAY:
            return build_slice_from_array(left.value.list, start, end);
        break;

        case OBJ_STRING:
            return build_slice_from_string(left.value.string, start, end);
        break;

        default:
            return make_error_object("Invalid object type for slice operation");
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
        &&GOTO_OPCODE_MODULO,
        &&GOTO_OPCODE_TRUE,
        &&GOTO_OPCODE_FALSE,
        &&GOTO_OPCODE_EQUAL,
        &&GOTO_OPCODE_NOT_EQUAL,
        &&GOTO_OPCODE_GREATER_THAN,
        &&GOTO_OPCODE_GREATER_THAN_OR_EQUALS,
        &&GOTO_OPCODE_LESS_THAN,
        &&GOTO_OPCODE_LESS_THAN_OR_EQUALS,
        &&GOTO_OPCODE_AND,
        &&GOTO_OPCODE_OR,
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
        &&GOTO_OPCODE_ARRAY,
        &&GOTO_OPCODE_INDEX_GET,
        &&GOTO_OPCODE_INDEX_SET,
        &&GOTO_OPCODE_SLICE,
        &&GOTO_OPCODE_HALT,
    };
    struct frame *frame = &vm_current_frame(vm);

    #ifdef DEBUG
    char *instruction_str = instruction_to_str(&frame->fn->instructions);
    printf("Executing VM!\nInstructions: %s\n", instruction_str);
    free(instruction_str);
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
        struct object condition = vm_stack_pop(vm);
        if (condition.type == OBJ_NULL || (condition.type == OBJ_BOOL && condition.value.boolean == false)) {
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

    GOTO_OPCODE_AND:
    GOTO_OPCODE_OR:
    GOTO_OPCODE_ADD:
    GOTO_OPCODE_SUBTRACT:
    GOTO_OPCODE_MULTIPLY:
    GOTO_OPCODE_DIVIDE:
    GOTO_OPCODE_MODULO: {
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
    GOTO_OPCODE_GREATER_THAN_OR_EQUALS:
    GOTO_OPCODE_LESS_THAN: 
    GOTO_OPCODE_LESS_THAN_OR_EQUALS: {
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
        vm_stack_push(vm, get_builtin_by_index(idx));
        DISPATCH();
    }

    GOTO_OPCODE_ARRAY: {
        uint16_t num_elements = read_uint16((frame->ip + 1));
        frame->ip += 3;
        struct object array = vm_build_array(vm, vm->stack_pointer - num_elements, vm->stack_pointer);
        vm->stack_pointer -= num_elements;
        vm_stack_push(vm, array);
        gc_add(vm, array);
        DISPATCH();
    }

    GOTO_OPCODE_SLICE: {
        struct object end = vm_stack_pop(vm);
        struct object start = vm_stack_pop(vm);
        struct object left = vm_stack_pop(vm);
        struct object obj = build_slice(left, start, end);
        gc_add(vm, obj);
        vm_stack_push(vm, obj);
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_INDEX_GET: {
        struct object index = vm_stack_pop(vm);
        struct object left = vm_stack_pop(vm);
        frame->ip++;

        if (index.type != OBJ_INT) {
            struct object obj = make_error_object("Array index must be integer or slice");
            vm_stack_push(vm, obj);
            gc_add(vm, obj);
            DISPATCH();
        }

        switch (left.type) {
            case OBJ_ARRAY: {
                struct object_list* list = left.value.list;
                unsigned idx = (unsigned) (index.value.integer < 0 ? list->size + index.value.integer : index.value.integer);
                if (idx >= list->size) {
                    vm_stack_push(vm, make_error_object("Array index out of bounds"));
                    gc_add(vm, vm_stack_cur(vm));
                } else {
                    vm_stack_push(vm, list->values[idx]);
                }
            }
            break;

            case OBJ_STRING: {
                const char *str = left.value.string->value;
                unsigned idx = (unsigned) (index.value.integer < 0 ? (int) left.value.string->length + index.value.integer : index.value.integer);
                if (idx >= left.value.string->length) {
                    vm_stack_push(vm, make_error_object("String index out of bounds"));
                    gc_add(vm, vm_stack_cur(vm));
                } else {
                    /* TODO: Create char object? Bit wasteful here for a single byte */
                    char buf[2];
                    buf[0] = (char) str[idx];
                    buf[1] = '\0';
                    struct object obj = make_string_object(buf);
                    vm_stack_push(vm, obj);
                    gc_add(vm, obj);
                }   
            }
            break;

            default: {
                struct object obj = make_error_object("Invalid left-hand side for indexing operation");
                vm_stack_push(vm, obj);
                gc_add(vm, obj);
            }
            break;
        }
        
        DISPATCH();
    }

    GOTO_OPCODE_INDEX_SET: {
        struct object value = vm_stack_pop(vm);
        struct object index = vm_stack_pop(vm);
        struct object array = vm_stack_pop(vm);
        assert(index.type == OBJ_INT);
        assert(array.type == OBJ_ARRAY);
        struct object_list* list = array.value.list;
        if (index.value.integer < 0 || index.value.integer >= list->size) {
            vm_stack_push(vm, make_error_object("Array assignment index out of bounds"));
            gc_add(vm, vm_stack_cur(vm));
        } else {
            list->values[index.value.integer] = copy_object(&value);

            // Push value on stack ???
            vm_stack_push(vm, value);
        }
        
        frame->ip++;
        DISPATCH();
    }

    GOTO_OPCODE_HALT: ;

    return VM_SUCCESS;
}

struct object vm_stack_last_popped(struct vm *vm) {
    return vm->stack[vm->stack_pointer];
}
