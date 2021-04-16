#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h> 
#include <sys/cdefs.h>
#include "util.h"
#include "opcode.h"
#include "object.h"
#include "parser.h"

struct object_list *object_list_pool_head = NULL;

const char *object_type_to_str(const enum object_type t) {
    const char *object_names[] = {
        "NULL",
        "BOOLEAN",
        "INTEGER",
        "BUILTIN",
        "ERROR",
        "STRING",
        "ARRAY",
        "COMPILED_FUNCTION",
    };
    return object_names[t];
}
  
struct object make_integer_object(const int64_t value) {
    return (struct object) {
        .type = OBJ_INT,
        .value.integer = value,
    };
}

struct object make_array_object(struct object_list *elements) {
    return (struct object) {
        .type = OBJ_ARRAY,
        .value.array = elements,
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
 
struct object copy_object(const struct object* restrict obj) {
    switch (obj->type) {
        case OBJ_BOOL:
        case OBJ_NULL:
        case OBJ_BUILTIN:
        case OBJ_INT:
            return *obj;
            break;

        case OBJ_ERROR: 
            return make_error_object(obj->value.error);
            break;
        
        case OBJ_STRING:
            return make_string_object(obj->value.string, NULL);
            break;

        case OBJ_ARRAY: 
            return make_array_object(obj->value.array);
            break;    

        case OBJ_COMPILED_FUNCTION: {
            struct instruction *ins = copy_instructions(&obj->value.compiled_function->instructions);
            return make_compiled_function_object(ins, obj->value.compiled_function->num_locals);
        }
        break;  

        default:
            err(EXIT_FAILURE, "unhandled object type passed to copy_object()");
        break;
    }
}

void free_object(struct object* restrict obj)
{   
    switch (obj->type) {
        case OBJ_NULL: 
        case OBJ_BOOL: 
        case OBJ_INT:
        case OBJ_BUILTIN:
            return;
            break;

        case OBJ_ERROR:
            free(obj->value.error);
            obj->value.error = NULL;
            break;

        case OBJ_ARRAY: 
            free_object_list(obj->value.array);
            obj->value.array = NULL;
            break;

        case OBJ_STRING: 
            free(obj->value.string);
            obj->value.string = NULL;
            break;

        case OBJ_COMPILED_FUNCTION: 
            free(obj->value.compiled_function->instructions.bytes);
            free(obj->value.compiled_function);
            obj->value.compiled_function = NULL;
        break;

       default:
           // nothing special
           break;
    }
}

struct object_list *make_object_list(uint32_t cap) {
    struct object_list *list;
    list = malloc(sizeof (*list));
    list->values = malloc(cap * sizeof(struct object));
    assert(list && list->values);
    list->size = 0;
    return list;
}

/* frees object list, incl all values contained in list */
void free_object_list(struct object_list *list) {
    for (uint32_t i=0; i < list->size; i++) {
        free_object(&list->values[i]);
    }
    free(list->values);
    free(list);
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

void object_to_str(char *str, const struct object obj)
{
    char tmp[128] = {0};

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
            strcat(str, obj.value.boolean ? "true" : "false");
            break;
            
        case OBJ_ERROR: 
            strcat(str, obj.value.error);
            break;  

        case OBJ_STRING: 
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
            strcat(str, obj.value.string);
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
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
