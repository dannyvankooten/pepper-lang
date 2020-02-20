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

struct object obj_null = {
    .type = OBJ_NULL
};

struct object obj_true = {
    .type = OBJ_BOOL,
    .boolean = { .value = 1 }
};
struct object obj_false = {
    .type = OBJ_BOOL,
    .boolean = { .value = 0 }
};

const char *object_type_to_str(enum object_type t);
struct object *eval_expression(struct expression *expr);
struct object *eval_block_statement(struct block_statement *block);

struct object *make_boolean_object(char value) {
    return value ? &obj_true : &obj_false;
}

struct object *make_integer_object(int value) {
    struct object *result = malloc(sizeof (struct object));
    result->type = OBJ_INT;
    result->integer.value = value;
    return result;
}

struct object *eval_bang_operator_expression(struct object *obj) {
    switch (obj->type) {
        case OBJ_BOOL: 
            return !obj->boolean.value ? &obj_true : &obj_false;
        break;
        case OBJ_NULL: 
            return &obj_true;
        break;
        default: 
            return &obj_false;
        break;
    }
}

struct object *eval_minus_prefix_operator_expression(struct object *obj) {
    if (obj->type != OBJ_INT) {
        return &obj_null;
    }

    return make_integer_object(-obj->integer.value);
}

struct object *eval_prefix_expression(operator operator, struct object *obj) {
    switch (operator[0]) {
        case '!':
            return eval_bang_operator_expression(obj);
            break;

        case '-':
            return eval_minus_prefix_operator_expression(obj);
            break;
    }

    return &obj_null; 
}


struct object *eval_integer_infix_expression(operator operator, struct object *left, struct object *right) {
    switch (operator[0]) {
        case '+':
            return make_integer_object(left->integer.value + right->integer.value);
        case '-':
            return make_integer_object(left->integer.value - right->integer.value);
        case '*':
            return make_integer_object(left->integer.value * right->integer.value);
        case '/':
            return make_integer_object(left->integer.value / right->integer.value);
        case '<':
            return make_boolean_object(left->integer.value < right->integer.value);
        case '>':
            return make_boolean_object(left->integer.value > right->integer.value);
        case '=':
            if (operator[1] == '=') {
                return make_boolean_object(left->integer.value == right->integer.value);
            }
            return &obj_null;
        case '!':
            if (operator[1] == '=') {
                return make_boolean_object(left->integer.value != right->integer.value);
            }    
            return &obj_null;
        default:
            return &obj_null;
            break;
    }
}

struct object *eval_infix_expression(operator operator, struct object *left, struct object *right) {
    if (left->type == OBJ_INT && right->type == OBJ_INT) {
        return eval_integer_infix_expression(operator, left, right);
    }

    if (left->type == OBJ_BOOL && right->type == OBJ_BOOL) {
        if (operator[0] == '=' && operator[1] == '=') {            
            return make_boolean_object(left == right);
        }

        if (operator[0] == '!' && operator[1] == '=') {
            return make_boolean_object(left != right);
        }
    }

    return &obj_null; 
}

unsigned char is_object_truthy(struct object *obj) {
    if (obj == &obj_null || obj == &obj_false) {
        return 0;
    }

    return 1;
}

struct object *eval_if_expression(struct if_expression *expr) {
    struct object *obj = eval_expression(expr->condition);

    if (is_object_truthy(obj)) {
        return eval_block_statement(expr->consequence);
    } else if (expr->alternative) {
        return eval_block_statement(expr->alternative);
    }

    // TODO: Free up integer objects

    return &obj_null;
}

struct object *eval_expression(struct expression *expr) {
    struct object *left = NULL;
    struct object *right = NULL;
    struct object *result = NULL;

    switch (expr->type) {
        case EXPR_INT:
            return make_integer_object(expr->integer.value);
        case EXPR_BOOL: 
            return make_boolean_object(expr->bool.value);
        case EXPR_PREFIX: 
            right = eval_expression(expr->prefix.right);
            result = eval_prefix_expression(expr->prefix.operator, right);
            return result;
        case EXPR_INFIX:
            left = eval_expression(expr->infix.left);
            right = eval_expression(expr->infix.right);
            result = eval_infix_expression(expr->infix.operator, left, right);
            return result;
        case EXPR_IF:
            return eval_if_expression(&expr->ifelse);
            break;
    }

    // TODO: Free pointers (to integer objects only)

    return &obj_null;
}



struct object *eval_statement(struct statement *stmt) {
    switch (stmt->type) {
        case STMT_EXPR:
            return eval_expression(stmt->value);
            break;

    }

    return &obj_null;
};

struct object *eval_block_statement(struct block_statement *block) {
    struct object *obj;

    for (int i=0; i < block->size; i++) {
        obj = eval_statement(&block->statements[i]);
    }

    return obj;
}

struct object *eval_program(struct program *prog) {
    struct object *obj;

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
            strcat(str, obj == &obj_true ? "true" : "false");
            break;
    }
}

const char *object_type_to_str(enum object_type t) {
    switch (t) {
        case OBJ_BOOL: return "OBJ_BOOL";
        case OBJ_INT: return "OBJ_INT";
        case OBJ_NULL: return "OBJ_NULL";
        default: return "Invalid object type";
    }
}