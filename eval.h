#include "parser.h"
#include <stdlib.h> 
#include <string.h> 
#include <stdio.h>

enum object_type {
    OBJ_NULL,
    OBJ_BOOL,
    OBJ_INT,
};

struct boolean {
    unsigned char value;
};

struct integer {
    int value;
};

struct null {};

struct object {
    enum object_type type;
    union {
        struct boolean boolean;
        struct integer integer;
        struct null null;
    };
};

const struct object obj_null = {
    .type = OBJ_NULL
};

const struct object obj_true = {
    .type = OBJ_BOOL,
    .boolean = { .value = 1 }
};
const struct object obj_false = {
    .type = OBJ_BOOL,
    .boolean = { .value = 0 }
};

struct object eval_expression(struct expression *expr) {
    struct object obj;
    switch(expr->type) {
        case EXPR_INT:
            obj.type = OBJ_INT;
            obj.integer.value = expr->integer.value;
            break;
        case EXPR_BOOL: 
            return expr->bool.value ? obj_true : obj_false;
            break;
    }

    return obj;
}

struct object eval_statement(struct statement *stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            return eval_expression(stmt->value);
        break;
    }

    return obj_null;
};

struct object eval_program(struct program *prog) {
    struct object obj;
    for (int i=0; i < prog->size; i++) {
        obj = eval_statement(&prog->statements[i]);
    }
    return obj;
}

void object_to_str(char *str, struct object *obj) {
    char tmp[16];

    switch (obj->type) {
        case OBJ_NULL:
            strcat(str, "NULL");
            break;
        case OBJ_INT:
            sprintf(tmp, "%d", obj->integer.value);
            strcat(str, tmp);
            break;
        case OBJ_BOOL:
            strcat(str, obj->boolean.value ? "true" : "false");
            break;
    }
}