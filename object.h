#ifndef OBJECT_H
#define OBJECT_H

#include <err.h> 
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 

struct object_list_pool {
    struct object_list *head;
};

struct object_list_pool object_list_pool = {
    .head = NULL,
};

struct object_pool {
    struct object *head;
};

struct object_pool object_pool = {
    .head = NULL,
};

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
    char *name;
    union {
        unsigned char boolean;
        long integer;
        char *error;
        struct function function;
    };
    unsigned char return_value;
    struct object *next;
};

struct object_list {
    struct object **values;
    unsigned int size;
    unsigned int cap;

    // for linking in pool
    struct object_list *next;
};

static struct object _object_null = {
    .type = OBJ_NULL,
    .return_value = 0
};
static struct object _object_null_return = {
    .type = OBJ_NULL,
    .return_value = 1
};
static struct object _object_true = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 0
};
static struct object _object_false = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 0
};
static struct object _object_true_return = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 1
};
static struct object _object_false_return = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 1
};
struct object *object_null = &_object_null;
struct object *object_null_return = &_object_null_return;
struct object *object_true = &_object_true;
struct object *object_false = &_object_false;
struct object *object_true_return = &_object_true_return;
struct object *object_false_return = &_object_false_return;

void free_object(struct object *obj);
const char *object_type_to_str(enum object_type t);

const char *object_type_to_str(enum object_type t)
{
    return object_names[t];
}

struct object *make_boolean_object(char value)
{
    return value ? object_true : object_false;
}

struct object *make_object() {
    struct object *obj;

    // try to get pre-allocated object from pool
    if (!object_pool.head) {
        obj = malloc(sizeof *obj);
        if (!obj) {
            errx(EXIT_FAILURE, "out of memory");
        }
    } else {
        obj = object_pool.head;
        object_pool.head = obj->next;
    }

    obj->next = NULL;
    return obj;
}

struct object *make_integer_object(long value)
{
    struct object *obj = make_object();
    obj->type = OBJ_INT;
    obj->integer = value;
    obj->return_value = 0;
    return obj;
}

struct object *make_error_object(char *format, ...) {
    va_list args;

    struct object *obj = malloc(sizeof *obj);
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
}

struct object *make_function_object(struct identifier_list *parameters, struct block_statement *body, struct environment *env) {
    struct object *obj = make_object();
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
        case OBJ_INT:
            // return object to pool so future calls of make_object can use it
            obj->next = object_pool.head;
            object_pool.head = obj;
            break;

    }
}

void free_object_pool() {
    struct object *obj = object_pool.head;
    struct object *next = NULL;
    while (obj) {
        next = obj->next;
        free(obj);
        obj = next;
    }
}

#endif