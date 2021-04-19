#pragma once 

#include <stdbool.h>
#include <stdint.h>
#include "parser.h"
#include "opcode.h"

// TODO: Dynamically allocate this
#define OBJECT_LIST_MAX_VALUES 512

enum object_type
{
    OBJ_NULL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_BUILTIN,
    OBJ_ERROR,
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_COMPILED_FUNCTION,
};

struct function {
    struct identifier_list *parameters;
    struct block_statement *body;
    struct environment *env;
};

struct compiled_function {
    struct instruction instructions;
    uint32_t num_locals;
};

struct heap_object {
    void *value;

    /* for gc */
    bool marked;
};

union object_value {
    bool boolean;
    int64_t integer;
    struct heap_object* ptr;
};

struct object
{
    enum object_type type;
    union object_value value;
};

struct object_list {
    struct object* values;
    uint32_t size;
};


const char *object_type_to_str(const enum object_type t);
struct object make_integer_object(const int64_t value);
struct object make_string_object(const char *str1, const char *str2);
struct object make_error_object(const char *format, ...);
struct object make_array_object(struct object_list *elements);
struct object make_compiled_function_object(struct instruction *ins, const uint32_t num_locals);

struct object copy_object(const struct object* obj);
void free_object(struct object* obj);
void object_to_str(char *str, struct object obj);

struct object_list *make_object_list(uint32_t cap);
struct object_list *copy_object_list(const struct object_list *original);
void free_object_list(struct object_list *list);
