#include <stdarg.h>
#include <string.h> 
#include <stdio.h>
#include "object.h"
#include "builtins.h"

struct object *builtin_len(struct object_list * args);
struct object *builtin_puts(struct object_list * args);

struct object *get_builtin(char *name) {
    static struct object len = {
        .type = OBJ_BUILTIN,
        .value = { .builtin = &builtin_len }
    };

    static struct object puts = {
        .type = OBJ_BUILTIN,
        .value = { .builtin = &builtin_puts }
    };

    if (strcmp(name, "len") == 0) {
        return &len;
    } else if (strcmp(name, "puts") == 0) {
        return &puts;
    }

    return NULL;
}

struct object *builtin_len(struct object_list * args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    struct object *arg = args->values[0];
    if (arg->type != OBJ_STRING) {
        return make_error_object("argument to len() not supported: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(arg->type));
    }

    return make_integer_object(strlen(arg->value.string));
}

struct object *builtin_puts(struct object_list * args) {
    for (int i=0; i < args->size; i++) {
        char str[1024] = {'\0'};
        object_to_str(str, args->values[i]);
        printf("%s", str);
    }
    printf("\n");

    return object_null;
}
