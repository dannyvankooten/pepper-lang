#pragma once 

#include <stdbool.h>
#include <stdint.h>
#include "parser.h"
#include "opcode.h"

// TODO: Dynamically allocate this
#define OBJECT_LIST_MAX_VALUES 512

enum object_type
{
    OBJ_NULL,               // 0b000
    OBJ_BOOL,               // 0b001
    OBJ_INT,                // 0b010
    OBJ_BUILTIN,            // 0b011
    OBJ_ERROR,              // 0b100
    OBJ_STRING,             // 0b101
    OBJ_ARRAY,              // 0b110
    OBJ_COMPILED_FUNCTION,  // 0b111
};

struct function {
    struct identifier_list *parameters;
    struct block_statement *body;
    struct environment *env;
};

struct gc_meta {
    bool marked;
};

struct compiled_function {
    struct instruction instructions;
    uint32_t num_locals;
    struct gc_meta gc_meta;
};

struct string {
    char *value;
    size_t length;
    size_t cap;
    struct gc_meta gc_meta;
};

struct error {
    char *value;
    size_t length;
    struct gc_meta gc_meta;
};

union object_value {
    bool boolean;
    int64_t integer;
	struct object (*fn_builtin)(const struct object_list* args);
    /* TODO: Remove void pointer */
    void *value;
    struct error *error;
    struct object_list* list;
    struct compiled_function* fn_compiled;
    struct string* string;
};

struct object
{
    enum object_type type;
    union object_value value;
};

struct object_list {
    struct object* values;
    uint32_t size;
    uint32_t cap;
    struct gc_meta gc_meta;
};

const char *object_type_to_str(const enum object_type t);
struct object make_integer_object(const int64_t value);
struct object make_boolean_object(const bool value);
struct object make_string_object(const char *str1);
struct object make_string_object_with_length(const char *str, size_t length);
struct object make_error_object(const char *format, ...);
struct object make_array_object(struct object_list *elements);
struct object make_compiled_function_object(const struct instruction *ins, uint32_t num_locals);
struct object concat_string_objects(struct string* left, struct string* right);
struct object copy_object(const struct object* obj);
void free_object(struct object* obj);
void object_to_str(char *str, struct object obj);
void print_object(struct object obj);
struct object_list *make_object_list(uint32_t cap);
void append_to_object_list(struct object_list* list, struct object obj);
struct object_list *copy_object_list(const struct object_list *original);
void free_object_list(struct object_list *list);
