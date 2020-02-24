#include <err.h> 
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include "object.h"
#include "parser.h"


struct object_pool {
    struct object *head;
};

struct object_pool object_pool = {
    .head = NULL,
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

static const char *object_names[] = {
    "NULL",
    "BOOLEAN",
    "INTEGER",
    "ERROR",
    "FUNCTION",
};

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
    // always return error objects
    obj->return_value = 1;
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

    // ensure environment isn't free'd up while we depend on it
    env->ref_count++;
    return obj;
}

struct object *copy_object(struct object *obj) {
    switch (obj->type) {
        case OBJ_BOOL:
        case OBJ_NULL:
            return obj;
            break;

        case OBJ_INT:
            return make_integer_object(obj->integer);
            break;

        case OBJ_FUNCTION:
            return make_function_object(obj->function.parameters, obj->function.body, obj->function.env);
            break;

        case OBJ_ERROR: 
            return make_error_object(obj->error);
            break;
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


unsigned char is_object_truthy(struct object *obj)
{
    if (obj == object_null || obj == object_false)
    {
        return 0;
    }

    return 1;
}

unsigned char is_object_error(enum object_type type) {
    return type == OBJ_ERROR;
}


void object_to_str(char *str, struct object *obj)
{
    char tmp[16];

    switch (obj->type)
    {
    case OBJ_NULL:
        strcat(str, "NULL");
        break;
    case OBJ_INT:
        sprintf(tmp, "%ld", obj->integer);
        strcat(str, tmp);
        break;
    case OBJ_BOOL:
        strcat(str, (obj == object_true  || obj == object_true_return) ? "true" : "false");
        break;
    case OBJ_ERROR: 
        strcat(str, obj->error);
        break;   
    case OBJ_FUNCTION: 
        strcat(str, "fn(");
        identifier_list_to_str(str, obj->function.parameters);
        strcat(str, ") {\n");
        block_statement_to_str(str, obj->function.body);
        strcat(str, "\n}");
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