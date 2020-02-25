#include <err.h> 
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include "object.h"
#include "parser.h"

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
struct object *_free_object_list = NULL;
struct object_list *_free_object_list_list = NULL;

static const char *object_names[] = {
    "NULL",
    "BOOLEAN",
    "INTEGER",
    "ERROR",
    "FUNCTION",
    "STRING",
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
   struct object *obj = _free_object_list;

   if (!obj) {
       obj = malloc(sizeof (*obj));

       if (!obj) {
           err(EXIT_FAILURE, "out of memory");
       }
   } else {
       _free_object_list = obj->next;
   }

    
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


struct object *make_string_object(char *str1, char *str2)
{
    struct object *obj = make_object();
    obj->type = OBJ_STRING;
    obj->return_value = 0;
    
    // allocate enough memory to fit both strings
    int l = strlen(str1) + (str2 ? strlen(str2) : 0) + 1;
    obj->string = malloc(l);
    if (!obj->string) {
        err(EXIT_FAILURE, "out of memory");
    }

    // piece strings together
    obj->string[0] = 0;
    strcat(obj->string, str1);
    if (str2) {
        strcat(obj->string, str2);
    }
   
    return obj;
}


struct object *make_error_object(char *format, ...) {
    va_list args;

    struct object *obj = make_object();
    obj->type = OBJ_ERROR;

    size_t l = strlen(format);
    obj->error = malloc(l + 16);
    if (!obj->error) {
        err(EXIT_FAILURE, "out of memory");
    }

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
        
        case OBJ_STRING:
            return make_string_object(obj->string, NULL);
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
            break;

        case OBJ_STRING: 
            free(obj->string);
            break;
        
       default:
           // nothing special
           break;
    }

     // add to start of free object list
    obj->next = _free_object_list;
    _free_object_list = obj;
    obj = NULL;
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


struct object_list *make_object_list(unsigned int cap) {
   struct object_list *list = _free_object_list_list;

   if (!list) {
       list = malloc(sizeof (*list));
       if (!list) {
           err(EXIT_FAILURE, "out of memory");
       }

       list->values = NULL;
       list->cap = 0;
   } else {
        _free_object_list_list = list->next;
   }

   if (list->cap < cap) {
        list->cap = cap;
        list->values = realloc(list->values, sizeof *list->values * cap);
        if (!list->values) {
            err(EXIT_FAILURE, "out of memory");
        }
    }

   return list;
}

void free_object_list(struct object_list *list) {
    list->next = _free_object_list_list;
    _free_object_list_list = list;
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

    case OBJ_STRING: 
        strcat(str, obj->string);
        break;
    }
}
