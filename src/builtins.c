#include "object.h"
#include "builtins.h"
#include <stdarg.h>
#include <string.h> 

struct object *builtin_len(struct object_list * args) {
    if (args->size != 1) {
        return make_error_object("wrong number of arguments: expected 1, got %d", args->size);
    }

    struct object *arg = args->values[0];
    if (arg->type != OBJ_STRING) {
        return make_error_object("argument to len() not supported: expected %s, got %s", object_type_to_str(OBJ_STRING), object_type_to_str(arg->type));
    }

    return make_integer_object(strlen(arg->string));
}

struct object *get_builtin(char *name) {
    static struct object len = {
        .type = OBJ_BUILTIN,
        .builtin = &builtin_len,
    };

    if (strcmp(name, "len") == 0) {
        return &len;
    }

    return NULL;
}