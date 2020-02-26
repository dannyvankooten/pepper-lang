#include <err.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include "object.h"
#include "parser.h"

struct object_list *copy_object_list(struct object_list *original);

static struct object _object_null = {
    .type = OBJ_NULL,
    .return_value = false,
};
static struct object _object_null_return = {
    .type = OBJ_NULL,
    .return_value = true,
};
static struct object _object_true = {
    .type = OBJ_BOOL,
    .boolean = true,
    .return_value = false,
};
static struct object _object_false = {
    .type = OBJ_BOOL,
    .boolean = false,
    .return_value = false,
};
static struct object _object_true_return = {
    .type = OBJ_BOOL,
    .boolean = true,
    .return_value = true,
};
static struct object _object_false_return = {
    .type = OBJ_BOOL,
    .boolean = false,
    .return_value = true,
};
struct object *object_null = &_object_null;
struct object *object_null_return = &_object_null_return;
struct object *object_true = &_object_true;
struct object *object_false = &_object_false;
struct object *object_true_return = &_object_true_return;
struct object *object_false_return = &_object_false_return;
struct object *object_pool_head = NULL;
struct object_list *object_list_pool_head = NULL;

static const char *object_names[] = {
    "NULL",
    "BOOLEAN",
    "INTEGER",
    "ERROR",
    "FUNCTION",
    "STRING",
    "BUILTIN",
    "ARRAY",
};

const char *object_type_to_str(enum object_type t)
{
    return object_names[t];
}


struct object *make_object(enum object_type type) {
   struct object *obj = object_pool_head;

   if (!obj) {
       obj = malloc(sizeof (*obj));

       if (!obj) {
           err(EXIT_FAILURE, "out of memory");
       }
   } else {
       object_pool_head = obj->next;
   }

   obj->type = type;
   obj->next = NULL;
   obj->name = NULL;
   obj->return_value = false;
   return obj;
}

struct object *make_integer_object(long value)
{
    struct object *obj = make_object(OBJ_INT);
    obj->integer = value;
    return obj;
}

struct object *make_array_object(struct object_list *elements) {
    struct object *obj = make_object(OBJ_ARRAY);
    obj->array = copy_object_list(elements);
    return obj;
}

struct object *make_string_object(char *str1, char *str2)
{
    struct object *obj = make_object(OBJ_STRING);
    
    // allocate enough memory to fit both strings
    int l = strlen(str1) + (str2 ? strlen(str2) : 0) + 1;
    obj->string = malloc(l);
    if (!obj->string) {
        err(EXIT_FAILURE, "out of memory");
    }

    // piece strings together
    strcpy(obj->string, str1);
    if (str2) {
        strcat(obj->string, str2);
    }
   
    return obj;
}


struct object *make_error_object(char *format, ...) {
    va_list args;

    struct object *obj = make_object(OBJ_ERROR);

    size_t l = strlen(format);
    obj->error = malloc(l + 64);
    if (!obj->error) {
        err(EXIT_FAILURE, "out of memory");
    }

    // always return error objects
    obj->return_value = true;
    va_start(args, format);  
    vsnprintf(obj->error, l + 64, format, args);
    va_end(args);
    return obj;
}


struct object *make_function_object(struct identifier_list *parameters, struct block_statement *body, struct environment *env) {
    struct object *obj = make_object(OBJ_FUNCTION);
    obj->function.parameters = parameters;
    obj->function.body = body;
    obj->function.env = env;
    return obj;
}


struct object *copy_object(struct object *obj) {
    switch (obj->type) {
        case OBJ_BOOL:
        case OBJ_NULL:
        case OBJ_BUILTIN:
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

        case OBJ_ARRAY: 
            return make_array_object(obj->array);
            break;
    }

    // TODO: This should not be reached, but also potential problem later on
    return obj;
}


void free_object(struct object *obj)
{   
    if (obj->name) {
        return;
    }

    switch (obj->type) {
        case OBJ_NULL: 
        case OBJ_BOOL: 
        case OBJ_BUILTIN:
            // never free static objects
            return;
            break;

        case OBJ_ERROR:
            free(obj->error);
            obj->error = NULL;
            break;

        case OBJ_ARRAY: 
            free_object_list(obj->array);
            obj->array = NULL;
            break;

        case OBJ_STRING: 
            free(obj->string);
            obj->string = NULL;
            break;

        case OBJ_FUNCTION:
            // free_environment(obj->function.env);
            // obj->function.env = NULL;
            break;

       default:
           // nothing special
           break;
    }

     // add to start of free object list
    obj->next = object_pool_head;
    object_pool_head = obj;
}


struct object_list *make_object_list(unsigned int cap) {
   struct object_list *list = object_list_pool_head;

   if (!list) {
       list = malloc(sizeof (*list));
       if (!list) {
           err(EXIT_FAILURE, "out of memory");
       }

       list->values = malloc(sizeof *list->values * cap);
       list->cap = cap;
       if (!list->values) {
            err(EXIT_FAILURE, "out of memory");
        }
   } else {
        object_list_pool_head = list->next;
        if (list->cap < cap) {
            list->cap = cap;
            list->values = realloc(list->values, sizeof *list->values * cap);
            if (!list->values) {
                err(EXIT_FAILURE, "out of memory");
            }
        }

        list->next = NULL;
   }

   list->size = 0;
   return list;
}

void free_object_list(struct object_list *list) {
    for (int i=0; i < list->size; i++) {
        free_object(list->values[i]);
    }

    list->next = object_list_pool_head;
    object_list_pool_head = list;
}


struct object_list *copy_object_list(struct object_list *original) {
    struct object_list *new = make_object_list(original->size);

    for (int i=0; i < original->size; i++) {
        new->values[i] = copy_object(original->values[i]);
        new->size++;
    }

    return new;
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

    case OBJ_BUILTIN: 
        strcat(str, "builtin function");
        break;    

    case OBJ_ARRAY: 
        strcat(str, "[");
        for (int i=0; i < obj->array->size; i++) {
            object_to_str(str, obj->array->values[i]);
            if (i < (obj->array->size - 1)) {
                strcat(str, ", ");
            }
        }
        strcat(str, "]");
    break;
    }
}

void free_object_pool() {
    struct object *node = object_pool_head;
    struct object *next = NULL;

    while (node) {
        next = node->next;
        free(node);
        node = next;
    }

    object_pool_head = NULL;
}

void free_object_list_pool() {
    struct object_list *node = object_list_pool_head;
    struct object_list *next = NULL;

    while (node) {
        next = node->next;
        free(node->values);
        free(node);
        node = next;
    }

    object_list_pool_head = NULL;
}