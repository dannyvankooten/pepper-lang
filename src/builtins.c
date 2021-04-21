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
static struct object builtin_puts(const struct object_list* args);
static struct object builtin_type(const struct object_list* args);
static struct object builtin_int(const struct object_list* args);
static struct object builtin_array_pop(struct object_list* args);
static struct object builtin_array_push(struct object_list* args);
static struct object builtin_file_get_contents(const struct object_list* args);
static struct object str_split(const struct object_list* args);


// here we store the built-in function directly on the pointer by casting it to the wrong value
// this saves us a level of indirection when calling built-in functions
const struct {
    const char* name;
    struct object fn_obj;
} builtin_functions[] = {
    { "puts", MAKE_BUILTIN(builtin_puts) },
    { "len", MAKE_BUILTIN(builtin_len) },
    { "type", MAKE_BUILTIN(builtin_type) },
    { "int", MAKE_BUILTIN(builtin_int) },
    { "array_pop", MAKE_BUILTIN(builtin_array_pop) },
    { "array_push", MAKE_BUILTIN(builtin_array_push) },
    { "file_get_contents", MAKE_BUILTIN(builtin_file_get_contents) },
    { "str_split", MAKE_BUILTIN(str_split) },
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
            return make_integer_object(strlen(arg.value.ptr->value));
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
            return make_integer_object(atoi(obj->value.ptr->value));
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

    char* buf = malloc(BUFSIZ);
    assert(buf != NULL);
    size_t size = 0;
    size_t read = 0;
    while ((read = fread(buf, sizeof(char), BUFSIZ, fd)) > 0) {
        size += read;

        if (read >= BUFSIZ) {
            buf = realloc(buf, size + BUFSIZ);
            assert(buf != NULL);
        }
    }
    buf[size] = '\0';
   
    struct object obj = make_string_object(buf, NULL);

    free(buf);
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

    char* str = (char*) args->values[0].value.ptr->value;
    const char* delim = (char*) args->values[1].value.ptr->value;
    const int delim_length = strlen(delim);
    struct object_list *list = make_object_list(2);

    char buf[BUFSIZ];
    char *src = str;
    char *dest = buf;
    while (*src != '\0') {
        if (strncmp(delim, src, delim_length) == 0) {
            *dest++ = '\0';
            list = append_to_object_list(list, make_string_object(buf, NULL));
            dest = buf;
            src += delim_length; // skip delim
        } else {
            *dest++ = *src++;
        }
    }

    *dest = '\0';
    list = append_to_object_list(list, make_string_object(buf, NULL));
    return make_array_object(list);
}