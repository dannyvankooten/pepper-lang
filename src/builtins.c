#include <stdarg.h>
#include <stdint.h>
#include <string.h> 
#include <stdio.h>
#include "object.h"
#include "builtins.h"

static struct object builtin_len(struct object_list * args);
static struct object builtin_puts(struct object_list * args);
static struct object builtin_type(struct object_list * args);
const struct object builtin_functions[] = {
    {
        .type = OBJ_BUILTIN,
        .value = { .builtin = &builtin_puts }
    },
    {
        .type = OBJ_BUILTIN,
        .value = { .builtin = &builtin_len }
    },
    {
        .type = OBJ_BUILTIN,
        .value = { .builtin = &builtin_type }
    }
};

inline 
struct object get_builtin_by_index(const uint8_t index) {
    return builtin_functions[index];
}

struct object get_builtin(const char* name) {
    if (strcmp(name, "puts") == 0) {
        return builtin_functions[0];
    } else if (strcmp(name, "len") == 0) {
        return builtin_functions[1];
    } else if (strcmp(name, "type") == 0) {
        return builtin_functions[2];
    }

    // TODO: Signal error
    return (struct object) {
        .type = OBJ_NULL,
    };
}

static struct object 
builtin_len(struct object_list* args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    struct object arg = args->values[0];
    if (arg.type != OBJ_STRING) {
        return make_error_object("argument to len() not supported: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(arg.type));
    }

    return make_integer_object(strlen(arg.value.string));
}

static struct object 
builtin_puts(struct object_list* args) {
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
builtin_type(struct object_list *args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }
    return make_string_object(object_type_to_str(args->values[0].type), NULL);
}