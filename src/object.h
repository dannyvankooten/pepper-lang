#ifndef OBJECT_H
#define OBJECT_H

#include "env.h"
#include "parser.h"

enum object_type
{
    OBJ_NULL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_ERROR,
    OBJ_FUNCTION,
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

struct object *object_null;
struct object *object_null_return;
struct object *object_true;
struct object *object_false;
struct object *object_true_return;
struct object *object_false_return;

const char *object_type_to_str(enum object_type t);
struct object *make_boolean_object(char value);
struct object *make_integer_object(long value);
struct object *make_error_object(char *format, ...);
struct object *make_function_object(struct identifier_list *parameters, struct block_statement *body, struct environment *env);
struct object *copy_object(struct object *obj);
void free_object(struct object *obj);
void free_object_pool();
void object_to_str(char *str, struct object *obj);
unsigned char is_object_truthy(struct object *obj);
unsigned char is_object_error(enum object_type type);

#endif