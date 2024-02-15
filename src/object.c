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

const char *object_type_to_str(enum object_type t) 
{
    static const char *object_names[] = {
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
  
struct object make_integer_object(int64_t value) 
{
    return (struct object) {
        .type = OBJ_INT,
        .value.integer = value,
    };
}

struct object make_boolean_object(bool value)
{
    return (struct object) {
        .type = OBJ_BOOL,
        .value.boolean = value
    };
}

struct object make_array_object(struct object_list *elements) 
{
    struct object obj;
    obj.type = OBJ_ARRAY;
    obj.value.list = elements;
    elements->gc_meta.marked = false;
    return obj;
}

struct object make_string_object_with_length(const char *str, size_t length)
{
    struct object obj;
    obj.type = OBJ_STRING;
    obj.value.string = malloc(sizeof(*obj.value.string) + length + 1);
    assert(obj.value.string != NULL);
    obj.value.string->gc_meta.marked = false;
    obj.value.string->value = (char*) (obj.value.string + 1);
    strcpy(obj.value.string->value, str);
    obj.value.string->length = length;
    return obj;
}

struct object make_string_object(const char *str)
{
    return make_string_object_with_length(str, strlen(str));
}

struct object concat_string_objects(struct string* left, struct string* right)
{
    struct object obj = make_string_object_with_length(left->value, left->length + right->length); 
    strcpy(obj.value.string->value + left->length, right->value);
    return obj;
}

struct object make_error_object(const char *format, ...) 
{
    va_list args;
    struct object obj;
    obj.type = OBJ_ERROR;

    // assume all expansions in the format string take up at most 64 bytes
    uint32_t len = strlen(format) + 64;
    obj.value.error = malloc(sizeof(*obj.value.error) + len);
    assert(obj.value.error != NULL);
    obj.value.error->gc_meta.marked = false;
    obj.value.error->value = (char*) (obj.value.error + 1);
    va_start(args, format);  
    vsnprintf(obj.value.error->value, len + 64, format, args);
    va_end(args);
    return obj;
}

struct object make_compiled_function_object(const struct instruction *ins, uint32_t num_locals) {
    struct object obj;
    obj.type = OBJ_COMPILED_FUNCTION;
    struct compiled_function *f = malloc(sizeof (struct compiled_function) + ins->size);
    assert(f != NULL);
    f->num_locals = num_locals;
    f->instructions.cap = ins->size;
    f->instructions.size = ins->size;
    f->instructions.bytes = (uint8_t *) (f + 1);
    memcpy(f->instructions.bytes, ins->bytes, ins->size);
    obj.value.fn_compiled = f;
    f->gc_meta.marked = false;
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
            return make_error_object(obj->value.error->value);
            break;
        
        case OBJ_STRING:
            return make_string_object(obj->value.string->value);
            break;

        case OBJ_ARRAY: {
            struct object_list* list = obj->value.list;
            return make_array_object(copy_object_list(list));
            break;    
        }

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* f = obj->value.fn_compiled;
            return make_compiled_function_object(&f->instructions, f->num_locals);
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
            free(obj->value.fn_compiled);
            break;
        }
        case OBJ_ARRAY: {
            // Note that we do not free the values in the list here
            // As these are also handled by the GC
            struct object_list* list = obj->value.list;
            free_object_list(list);
            break;
        }

        case OBJ_STRING:
            free(obj->value.string);
        break;

        case OBJ_ERROR:
            free(obj->value.error);
        break;

        default: 
            // see below
        break;
    }
}

struct object_list *make_object_list(uint32_t cap) {
    struct object_list *list;
    list = (struct object_list *) malloc(sizeof (struct object_list));
    assert(list != NULL);
    list->values = (struct object*) malloc(cap * sizeof(struct object));
    assert(list->values != NULL);
    list->cap = cap;
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

void 
append_to_object_list(struct object_list* list, struct object obj) {
    if (list->size == list->cap) {
        list->cap = (list->cap > 0) ? list->cap * 2 : 1;
        list->values = (struct object*) realloc(list->values, list->cap * sizeof(struct object));
        assert(list->values != NULL);
    }

    list->values[list->size++] = obj;
}

/* deep copy of object list, incl. all values */
struct object_list *copy_object_list(const struct object_list *original) {
    struct object_list *new = make_object_list(original->size);
    uint32_t size = original->size;
    for (uint32_t i=0; i < size; i++) {
        new->values[i] = copy_object(&original->values[i]);
    }
    new->size = size;
    return new;
}

void print_object(struct object obj) 
{
    switch (obj.type)
    {
        case OBJ_NULL:
            printf("null");
            break;
            
        case OBJ_INT:
            printf("%ld", obj.value.integer);
            break;
            
        case OBJ_BOOL:
            printf("%s", obj.value.boolean ? "true" : "false");
            break;
            
        case OBJ_ERROR: 
            printf("%s", obj.value.error->value);
            break;  

        case OBJ_STRING: 
            #ifdef DEBUG
                printf("\"%s\"", (const char *) obj.value.ptr->string.value);
            #else
                printf("%s", (const char *) obj.value.string->value);  
            #endif
            break;

        case OBJ_BUILTIN: 
            printf("builtin_function");
            break;    

        case OBJ_ARRAY: {
            printf("[");
            struct object_list* list = obj.value.list;
            for (uint32_t i=0; i < list->size; i++) {
                if (i > 0) {
                    printf(", ");
                }
                print_object(list->values[i]);
            }
            printf("]");
            break;
        }

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* f = obj.value.fn_compiled;
            char *instruction_str = instruction_to_str(&f->instructions);
            printf("%s", instruction_str);
            free(instruction_str);
            break;
        }
    }
}

// TODO: Fix this function for strings and arrays larger than BUFSIZ
void object_to_str(char *str, struct object obj)
{
    char tmp[BUFSIZ] = { '\0' };

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
            strcat(str, obj.value.error->value);
            break;  

        case OBJ_STRING: 
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
            strcat(str, obj.value.string->value);
            #ifdef DEBUG 
            strcat(str, "\"");
            #endif
            break;

        case OBJ_BUILTIN: 
            strcat(str, "builtin function");
            break;    

        case OBJ_ARRAY: {
            strcat(str, "[");
            struct object_list* list = obj.value.list;
            for (uint32_t i=0; i < list->size; i++) {
                object_to_str(str, list->values[i]);
                if (i < (list->size - 1)) {
                    strcat(str, ", ");
                }
            }
            strcat(str, "]");
            break;
        }

        case OBJ_COMPILED_FUNCTION: {
            struct compiled_function* f = obj.value.fn_compiled;
            char *instruction_str = instruction_to_str(&f->instructions);
            strcat(str, instruction_str);
            free(instruction_str);
            break;
        }
    }
}


