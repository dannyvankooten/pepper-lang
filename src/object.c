#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include "util.h"
#include "opcode.h"
#include "object.h"
#include "parser.h"

struct object *object_pool_head = NULL;
struct object_list *object_list_pool_head = NULL;

const char *object_type_to_str(enum object_type t) {
    const char *object_names[] = {
        "NULL",
        "BOOLEAN",
        "INTEGER",
        "ERROR",
        "STRING",
        "BUILTIN",
        "ARRAY",
        "COMPILED_FUNCTION",
    };
    return object_names[t];
}

struct object make_object(enum object_type type) {
    return (struct object) {
        .type = type,
    };
}

struct object make_integer_object(int64_t value)
{
    return (struct object) {
        .type = OBJ_INT,
        .value.integer = value,
    };
}

struct object make_array_object(struct object_list *elements) {
    return (struct object) {
        .type = OBJ_ARRAY,
        .value.array = copy_object_list(elements),
    };
}

struct object make_string_object(const char *str1, const char *str2)
{
    struct object obj;
    obj.type = OBJ_STRING;
    const uint32_t len = strlen(str1) + (str2 ? strlen(str2) : 0) + 1;
    obj.value.string = malloc(len);
    assert(obj.value.string != NULL);
    strcpy(obj.value.string, str1);
    if (str2) {
        strcat(obj.value.string, str2);
    }

    return obj;
}

struct object make_error_object(const char *format, ...) {
    va_list args;
    struct object obj;
    obj.type = OBJ_ERROR;

    uint32_t len = strlen(format);
    obj.value.error = malloc(len + 64);
    assert(obj.value.error != NULL);
    va_start(args, format);  
    vsnprintf(obj.value.error, len + 64, format, args);
    va_end(args);
    return obj;
}

struct object make_compiled_function_object(struct instruction *ins, uint32_t num_locals) {
    struct object obj;
    obj.type = OBJ_COMPILED_FUNCTION;
    struct compiled_function *f = malloc(sizeof (struct compiled_function));
    assert(f != NULL);
    f->num_locals = num_locals;
    f->instructions = *ins;
    free(ins);
    obj.value.compiled_function = f;
    return obj;
}   

struct object copy_object(struct object obj) {
    switch (obj.type) {
        case OBJ_BOOL:
        case OBJ_NULL:
        case OBJ_BUILTIN:
            return obj;
            break;

        case OBJ_INT:
            return make_integer_object(obj.value.integer);
            break;

        case OBJ_ERROR: 
            return make_error_object(obj.value.error);
            break;
        
        case OBJ_STRING:
            return make_string_object(obj.value.string, NULL);
            break;

        case OBJ_ARRAY: 
            return make_array_object(obj.value.array);
            break;

        case OBJ_COMPILED_FUNCTION:
            // TODO: Make copy here
            return obj;
            break;

        default:
            err(EXIT_FAILURE, "unhandled object type passed to copy_object()");
        break;
    }
}

/* return object related memory and return object itself to memory pool */
void free_object(struct object obj)
{   
    switch (obj.type) {
        case OBJ_NULL: 
        case OBJ_BOOL: 
        case OBJ_BUILTIN:
            // we re-use objects of this type since they contain no unique data
            // so we should not free anything here
            return;
            break;

        case OBJ_ERROR:
            free(obj.value.error);
            obj.value.error = NULL;
            break;

        case OBJ_ARRAY: 
            free_object_list(obj.value.array);
            obj.value.array = NULL;
            break;

        case OBJ_STRING: 
            free(obj.value.string);
            obj.value.string = NULL;
            break;

        case OBJ_COMPILED_FUNCTION: 
            free(obj.value.compiled_function->instructions.bytes);
            free(obj.value.compiled_function);
            obj.value.compiled_function = NULL;
        break;

       default:
           // nothing special
           break;
    }
}


struct object_list *make_object_list(uint32_t cap) {
   struct object_list *list = object_list_pool_head;

   if (!list) {
       list = malloc(sizeof (*list));
       if (!list) {
           err(EXIT_FAILURE, "out of memory");
       }
       list->size = 0;
   } else {
        object_list_pool_head = list->next;
        list->next = NULL;
   }

   return list;
}

/* returns object_list to pool, does not free values */
void free_object_list_shallow(struct object_list *list) {
    list->next = object_list_pool_head;
    object_list_pool_head = list;
    list->size = 0;
}

/* frees object list incl. all values contained in list */
void free_object_list(struct object_list *list) {
    for (uint32_t i=0; i < list->size; i++) {
        free_object(list->values[i]);
    }
    list->size = 0;

    free_object_list_shallow(list);
}


struct object_list *copy_object_list(struct object_list *original) {
    struct object_list *new = make_object_list(original->size);
    uint32_t size = original->size;
    for (uint32_t i=0; i < size; i++) {
        new->values[i] = original->values[i];
    }
    new->size = size;
    return new;
}

void object_to_str(char *str, struct object obj)
{
    char tmp[64] = {0};

    switch (obj.type)
    {
        case OBJ_NULL:
            strcat(str, "NULL");
            break;
            
        case OBJ_INT:
            sprintf(tmp, "%ld", obj.value.integer);
            strcat(str, tmp);
            break;
            
        case OBJ_BOOL:
            // We could do pointer comparison here, but since we've already followed the pointer above
            // It wouldn't really make a difference. So we check the actual value, as it's less error prone.
            strcat(str, obj.value.boolean ? "true" : "false");
            break;
            
        case OBJ_ERROR: 
            strcat(str, obj.value.error);
            break;  

        case OBJ_STRING: 
            strcat(str, obj.value.string);
            break;

        case OBJ_BUILTIN: 
            strcat(str, "builtin function");
            break;    

        case OBJ_ARRAY: {
            strcat(str, "[");
            for (uint32_t i=0; i < obj.value.array->size; i++) {
                object_to_str(str, obj.value.array->values[i]);
                if (i < (obj.value.array->size - 1)) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "]");
            break;
        }

        case OBJ_COMPILED_FUNCTION: {
            char *instruction_str = instruction_to_str(&obj.value.compiled_function->instructions);
            strcat(str, instruction_str);
            free(instruction_str);
            break;
        }
    }
}

void free_object_list_pool() {
    struct object_list *node = object_list_pool_head;
    struct object_list *next = NULL;

    while (node) {
        next = node->next;
        free(node);
        node = next;
    }

    object_list_pool_head = NULL;
}