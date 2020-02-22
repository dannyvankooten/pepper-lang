#ifndef OBJECT_H
#define OBJECT_H

#include <err.h> 
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 

enum object_type
{
    OBJ_NULL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_ERROR,
    OBJ_FUNCTION,
};

static const char *object_names[] = {
    "NULL",
    "BOOLEAN",
    "INTEGER",
    "ERROR",
    "FUNCTION",
};

struct function {
    struct identifier_list *parameters;
    struct block_statement *body;
    struct environment *env;
};

struct object
{
    enum object_type type;
    union {
        unsigned char boolean;
        long integer;
        char *error;
        struct function function;
    };
    unsigned char return_value;
};

struct object_list {
    struct object **values;
    unsigned int size;
    unsigned int cap;
};

struct object obj_null = {
    .type = OBJ_NULL,
    .return_value = 0
};
struct object obj_null_return = {
    .type = OBJ_NULL,
    .return_value = 1
};
struct object obj_true = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 0
};
struct object obj_false = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 0
};
struct object obj_true_return = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 1
};
struct object obj_false_return = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 1
};

void free_object(struct object *obj);
const char *object_type_to_str(enum object_type t);

const char *object_type_to_str(enum object_type t)
{
    return object_names[t] ?: "UNKOWN";
}

struct object *make_boolean_object(char value)
{
    return value ? &obj_true : &obj_false;
}

struct object *make_integer_object(long value)
{
    struct object *obj = malloc(sizeof (struct object));
    if (!obj) {
        errx(EXIT_FAILURE, "out of memory");
    }
    
    obj->type = OBJ_INT;
    obj->integer = value;
    obj->return_value = 0;
    return obj;
}

struct object *make_error_object(char *format, ...) {
    va_list args;

    struct object *obj = malloc(sizeof (struct object));
    if (!obj) {
        errx(EXIT_FAILURE, "out of memory");
    }

    size_t l = strlen(format);
    obj->error = malloc(l + 16);
    if (!obj->error) {
        errx(EXIT_FAILURE, "out of memory");
    }

    obj->type = OBJ_ERROR;
    obj->return_value = 0;
    va_start(args, format);  
    vsnprintf(obj->error, l + 16, format, args);
    va_end(args);
    return obj;
};

struct object *make_function_object(struct identifier_list *parameters, struct block_statement *body, struct environment *env) {
    struct object *obj = malloc(sizeof (struct object));
    if (!obj) {
        errx(EXIT_FAILURE, "out of memory");
    }

    obj->type = OBJ_FUNCTION;
    obj->return_value = 0;
    obj->function.parameters = parameters;
    obj->function.body = body;
    obj->function.env = env;
    return obj;
}

struct object *copy_object(struct object *obj) {
    switch (obj->type) {
        case OBJ_BOOL:
        case OBJ_NULL:
            return obj;

        case OBJ_INT:
            return make_integer_object(obj->integer);

        case OBJ_FUNCTION:
            return make_function_object(obj->function.parameters, obj->function.body, obj->function.env);

        case OBJ_ERROR: 
            return obj;
    }

    // TODO: This should not be reached, but also potential problem later on
    return obj;
}

void free_object(struct object *obj)
{   
    switch (obj->type) {
        case OBJ_NULL: 
        case OBJ_BOOL: 
            return;
            break;

        case OBJ_ERROR:
            free(obj->error);
            free(obj);
            break;
        
        case OBJ_FUNCTION:
            free(obj);
            break;

        case OBJ_INT:
            free(obj);
            break;

    }

    
}

#endif