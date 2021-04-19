#include <stdarg.h>
#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include "object.h"
#include "builtins.h"

#define MAKE_BUILTIN(fn) ((struct object) {                     \
    .type = OBJ_BUILTIN,                                        \
    .value = { .ptr = (struct heap_object *) &fn }    \
    }) 

static struct object builtin_len(const struct object_list* args);
static struct object builtin_puts(const struct object_list* args);
static struct object builtin_type(const struct object_list* args);
static struct object builtin_array_pop(struct object_list* args);
static struct object builtin_array_push(struct object_list* args);

// here we store the built-in function directly on the pointer by casting it to the wrong value
// this saves us a level of indirection when calling built-in functions
const struct {
    const char* name;
    struct object fn_obj;
} builtin_functions[] = {
    { "puts", MAKE_BUILTIN(builtin_puts) },
    { "len", MAKE_BUILTIN(builtin_len) },
    { "type", MAKE_BUILTIN(builtin_type) },
    { "array_pop", MAKE_BUILTIN(builtin_array_pop) },
    { "array_push", MAKE_BUILTIN(builtin_array_push) },
};

inline 
struct object get_builtin_by_index(const uint8_t index) {
    return builtin_functions[index].fn_obj;
}

struct object get_builtin(const char* name) {
    for (int i = 0; i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); i++) {
        if (strcmp(name, builtin_functions[i].name) == 0) {
            return builtin_functions[i].fn_obj;
        }
    }
    
    return (struct object) {
        .type = OBJ_NULL,
    };
}


// defines built-in functions in compiler symbol table
void define_builtins(struct symbol_table* st) {
    for (int i = 0; i < sizeof(builtin_functions) / sizeof(builtin_functions[0]); i++) {
        symbol_table_define_builtin_function(st, i, builtin_functions[i].name);
    }
}

static struct object 
builtin_len(const struct object_list* args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    struct object arg = args->values[0];
    if (arg.type != OBJ_STRING) {
        return make_error_object("argument to len() not supported: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(arg.type));
    }

    return make_integer_object(strlen(arg.value.ptr->value));
}

static struct object 
builtin_puts(const struct object_list* args) {
    char str[BUFSIZ];
    for (uint32_t i=0; i < args->size; i++) {
        *str = '\0';
        object_to_str(str, args->values[i]);
        printf("%s", str);
    }
    printf("\n");

    return (struct object) {
        .type = OBJ_NULL,
    };
}

static struct object 
builtin_type(const struct object_list *args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }
    return make_string_object(object_type_to_str(args->values[0].type), NULL);
}

static struct object 
builtin_array_pop(struct object_list *args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    if (args->values[0].type != OBJ_ARRAY) {
        return make_error_object("invalid argument: expected ARRAY, got %s", object_type_to_str(args->values[0].type));
    }

    struct object_list *arr = (struct object_list *) args->values[0].value.ptr->value;
    if (arr->size == 0) {
        return (struct object) {.type = OBJ_NULL};
    }

    return copy_object(&arr->values[arr->size-- - 1]);
}

static struct object 
builtin_array_push(struct object_list *args) {
    if (args->size != 2) {
        return make_error_object("wrong number of arguments: expected 2, got %d", args->size);
    }

    if (args->values[0].type != OBJ_ARRAY) {
        return make_error_object("invalid argument: expected ARRAY, got %s", object_type_to_str(args->values[0].type));
    }
    
    struct object_list *arr = (struct object_list *) args->values[0].value.ptr->value;
    struct object *value = (struct object *) &args->values[1];

    // grow list capacity if needed
    if (arr->size == arr->cap) {
        arr->cap = arr->cap > 0 ? arr->cap * 2 : 1;
        arr = (struct object_list*) realloc(arr, sizeof(struct object_list) + arr->cap * sizeof(struct object));
        arr->values = (struct object*) (arr + 1);
        args->values[0].value.ptr->value = arr;
    }

    arr->values[arr->size++] = copy_object(value);

    // return new size of array
    return make_integer_object(arr->size);
}