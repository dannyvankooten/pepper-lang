#include <assert.h>
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
static struct object builtin_print(const struct object_list* args);
static struct object builtin_type(const struct object_list* args);
static struct object builtin_int(const struct object_list* args);
static struct object builtin_array_pop(struct object_list* args);
static struct object builtin_array_push(struct object_list* args);
static struct object builtin_file_get_contents(const struct object_list* args);
static struct object str_split(const struct object_list* args);
static struct object str_contains(const struct object_list* args);


// here we store the built-in function directly on the pointer by casting it to the wrong value
// this saves us a level of indirection when calling built-in functions
const struct {
    const char* name;
    struct object fn_obj;
} builtin_functions[] = {
    { "print", MAKE_BUILTIN(builtin_print) },
    { "len", MAKE_BUILTIN(builtin_len) },
    { "type", MAKE_BUILTIN(builtin_type) },
    { "int", MAKE_BUILTIN(builtin_int) },
    { "array_pop", MAKE_BUILTIN(builtin_array_pop) },
    { "array_push", MAKE_BUILTIN(builtin_array_push) },
    { "file_get_contents", MAKE_BUILTIN(builtin_file_get_contents) },
    { "str_split", MAKE_BUILTIN(str_split) },
    { "str_contains", MAKE_BUILTIN(str_contains) }
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
    switch (arg.type) {
        case OBJ_STRING:
            return make_integer_object(arg.value.ptr->string.length);
        break;

        case OBJ_ARRAY:
            return make_integer_object(((struct object_list*) (arg.value.ptr->value))->size);
        break;

        default:
            return make_error_object("argument to len() not supported: got %s", object_type_to_str(arg.type));
        break;
    }

}

static struct object 
builtin_print(const struct object_list* args) {
    for (uint32_t i=0; i < args->size; i++) {
        print_object(args->values[i]);
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
    return make_string_object(object_type_to_str(args->values[0].type));
}

static struct object 
builtin_int(const struct object_list *args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    struct object* obj = &args->values[0];
    switch (obj->type) {
        case OBJ_INT:
            return *obj;
        break;

        case OBJ_STRING:
            return make_integer_object(atoi(obj->value.ptr->string.value));
        break;

        case OBJ_BOOL:
            return make_integer_object(obj->value.boolean ? 1 : 0);
        break;

        default:
            
        break;
    }

    return make_error_object("invalid object type");
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
    
    struct object_list **arr = (struct object_list **) &args->values[0].value.ptr->value;
    struct object *value = (struct object *) &args->values[1];
    *arr = append_to_object_list(*arr, copy_object(value)); 
    return make_integer_object((*arr)->size);
}

static struct object 
builtin_file_get_contents(const struct object_list *args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    if (args->values[0].type != OBJ_STRING) {
        return make_error_object("invalid argument: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(args->values[0].type));
    }

    const char *filename = (char *) args->values[0].value.ptr->value;
    FILE *fd = fopen(filename, "rb");
    if (!fd) {
        return make_error_object("error opening file \"%s\"", filename);
    }

    fseek(fd, 0, SEEK_END);
    size_t fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET); 

    struct object obj = make_string_object_with_length("", fsize);
    size_t bytes_read = fread(obj.value.ptr->string.value, 1, fsize, fd);
    assert(bytes_read >= 0);
    obj.value.ptr->string.value[fsize] = '\0';
    fclose(fd);
    return obj;
}

static struct object 
str_split(const struct object_list *args) {
    if (args->size != 2) {
        return make_error_object("wrong number of arguments: expected 2, got %d", args->size);
    }

    if (args->values[0].type != OBJ_STRING || args->values[1].type != OBJ_STRING) {
        return make_error_object("invalid argument: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(args->values[0].type));
    }

    const char* str = (char*) args->values[0].value.ptr->value;
    const char* delim = (char*) args->values[1].value.ptr->value;
    const int delim_length = strlen(delim);
    struct object_list *list = make_object_list(8);

    char buf[BUFSIZ];
    char *p;

    while ((p = strstr(str, delim)) != NULL) {
        size_t len = p - str;
        memcpy(buf, str, len);
        buf[len] = '\0';
        list = append_to_object_list(list, make_string_object_with_length(buf, len));
        str = p + delim_length;
    }
    
    // remainder (after last delimiter)
    strcpy(buf, str);
    list = append_to_object_list(list, make_string_object(buf));
    return make_array_object(list);
}

static struct object 
str_contains(const struct object_list *args) {
    if (args->size != 2) {
        return make_error_object("wrong number of arguments: expected 2, got %d", args->size);
    }

    if (args->values[0].type != OBJ_STRING || args->values[1].type != OBJ_STRING) {
        return make_error_object("invalid argument: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(args->values[0].type));
    }

    const char* subject = (char*) args->values[0].value.ptr->value;
    const char* search = (char*) args->values[1].value.ptr->value;
    char* ret;

    ret = strstr(subject, search);

    return make_boolean_object(ret != NULL);
}