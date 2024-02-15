#include <stdbool.h>
#include "vm.h"
#include "object.h"
#include "gc.h"

void 
gc_add(struct vm* restrict vm, struct object obj) {
    if (obj.type <= OBJ_BUILTIN) {
        return;
    }

    gc(vm);
    append_to_object_list(vm->heap, obj);
}

void 
gc_mark(struct object* obj, bool marked) {
    switch (obj->type) {
        case OBJ_STRING:
            obj->value.string->gc_meta.marked = marked;
        break;

        case OBJ_COMPILED_FUNCTION:
            obj->value.fn_compiled->gc_meta.marked = marked;
        break;

        case OBJ_ARRAY:
            obj->value.list->gc_meta.marked = marked;
        break;

        default: break;
    }
}

void 
gc_sweep(struct object_list* heap) {
     // traverse all objects, free all unmarked objects
    for (int32_t i = heap->size - 1; i >= 0; i--) {
        bool marked = false;
        struct object* obj = &heap->values[i];
        switch (obj->type) {
            case OBJ_STRING:
                marked = obj->value.string->gc_meta.marked;
            break;

            case OBJ_COMPILED_FUNCTION:
                marked = obj->value.fn_compiled->gc_meta.marked;
            break;

            case OBJ_ARRAY:
                marked = obj->value.list->gc_meta.marked;
            break;

            default: 
                // skip stack allocated objects and built-in functions
                continue;
            break;
        }

        if (marked) {
            gc_mark(obj, false);
        } else {
            free_object(obj);
            heap->values[i] = heap->values[--heap->size];
        }
    }
}

void 
gc(struct vm* restrict vm) 
{
    // we want to run the garbage collector pretty much all the time when in debug mode
    // so this code gets properly exercised
    #ifndef TEST_MODE 
    if (vm->heap->size < (vm->heap->cap * 0.8)) {
        return;
    }
    #endif 

    #ifdef DEBUG_GC
    printf("GARBAGE COLLECTION START\n");
    printf("Heap size (before): %d\n", vm->heap->size);
    #endif

    // traverse VM constants, stack and globals and mark every object that is reachable
    for (uint32_t i=0; i < vm->stack_pointer; i++) {
        gc_mark(&vm->stack[i], true);
    }
    for (uint32_t i=0; i < vm->nconstants; i++) {
        gc_mark(&vm->constants[i], true);
    }
    for (uint32_t i=0; i < GLOBALS_SIZE && vm->globals[i].type != OBJ_NULL; i++) {
        gc_mark(&vm->globals[i], true);
    }

    gc_sweep(vm->heap);

    #ifdef DEBUG_GC
    printf("Heap size (after): %d\n", vm->heap->size);
    printf("GARBAGE COLLECTION DONE\n");
    #endif
}
