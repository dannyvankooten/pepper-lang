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

const char *object_type_to_str(const enum object_type t) 
{
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
  
struct object make_integer_object(const int64_t value) 
{
    return (struct object) {
        .type = OBJ_INT,
        .value.integer = value,
    };
}

struct object make_array_object(struct object_list *elements) 
{
    struct object obj;
    obj.type = OBJ_ARRAY;
    obj.value.ptr = malloc(sizeof(*obj.value.ptr));
    assert(obj.value.ptr != NULL);
    obj.value.ptr->marked = false;
    obj.value.ptr->value = elements;
    return obj;
}

struct object make_string_object(const char *str1, const char *str2)
{
    struct object obj;
    obj.type = OBJ_STRING;

    const uint32_t len = strlen(str1) + (str2 ? strlen(str2) : 0) + 1;

    obj.value.ptr = malloc(sizeof(*obj.value.ptr) + len);
    assert(obj.value.ptr != NULL);
    obj.value.ptr->marked = false;
    obj.value.ptr->value = obj.value.ptr + 1;
    assert(obj.value.ptr->value != NULL);

    strcpy(obj.value.ptr->value, str1);
    if (str2) {
        strcat(obj.value.ptr->value, str2);
    }

    return obj;
}

struct object make_error_object(const char *format, ...) 
{
    va_list args;
    struct object obj;
    obj.type = OBJ_ERROR;

    // assume all expansions in the format string take up at most 64 bytes
    uint32_t len = strlen(format) + 64;
    obj.value.ptr = malloc(sizeof(*obj.value.ptr) + len);
    assert(obj.value.ptr != NULL);
    obj.value.ptr->marked = false;
    obj.value.ptr->value = obj.value.ptr + 1;
    assert(obj.value.ptr->value != NULL);
    va_start(args, format);  
    vsnprintf(obj.value.ptr->value, len + 64, format, args);
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
    obj.value.ptr = malloc(sizeof(*obj.value.ptr));
    assert(obj.value.ptr != NULL);
    obj.value.ptr->value = f;
    return obj;
}   
 
 // deep copy, incl. all children and values pointed to
struct object copy_object(const struct object* restrict obj) {
    switch (obj->type) {
        case OBJ_BOOL:
        case OBJ_NULL:
        case OBJ_BUILTIN:
        case OBJ_INT:
            // these values contain no pointers, so we can just dereference them
            return *obj;
            break;

        case OBJ_ERROR: 
            return make_error_object(obj->value.ptr->value);
            break;
        
        case OBJ_STRING:
            return make_string_object(obj->value.ptr->value, NULL);
            break;

        case OBJ_ARRAY: {
            struct object_list* list = (struct object_list*) obj->value.ptr->value;
            return make_array_object(copy_object_list(list));
            break;    
        }

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* f = (struct compiled_function*) obj->value.ptr->value;
            struct instruction *ins = copy_instructions(&f->instructions);
            return make_compiled_function_object(ins, f->num_locals);
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

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* fn = (struct compiled_function*) obj->value.ptr->value;
            free(fn->instructions.bytes);
            free(obj->value.ptr->value);
            break;
        }
        case OBJ_ARRAY: {
            // Note that we do not free the values in the list here
            // As these are also handled by the GC
            struct object_list* list = (struct object_list*) obj->value.ptr->value;
            free_object_list(list);
            break;
        }

        default: 
            // see below
        break;
    }

    free(obj->value.ptr);
    obj->value.ptr = NULL;
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

/* deep copy of object list, incl. all values */
struct object_list *copy_object_list(struct object_list *original) {
    struct object_list *new = make_object_list(original->size);
    uint32_t size = original->size;
    for (uint32_t i=0; i < size; i++) {
        new->values[i] = copy_object(&original->values[i]);
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
            strcat(str, obj.value.ptr->value);
            break;  

        case OBJ_STRING: 
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
            strcat(str, obj.value.ptr->value);
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
            break;

        case OBJ_BUILTIN: 
            strcat(str, "builtin function");
            break;    

        case OBJ_ARRAY: {
            strcat(str, "[");
            struct object_list* arr = (struct object_list*) obj.value.ptr->value;
            for (uint32_t i=0; i < arr->size; i++) {
                object_to_str(str, arr->values[i]);
                if (i < (arr->size - 1)) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "]");
            break;
        }

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* f = (struct compiled_function*) obj.value.ptr->value;
            char *instruction_str = instruction_to_str(&f->instructions);
            strcat(str, instruction_str);
            free(instruction_str);
            break;
        }
    }
}
