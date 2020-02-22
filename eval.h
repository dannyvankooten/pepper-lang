#ifndef EVAL_H
#define EVAL_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <err.h>

#include "gc.h"
#include "parser.h"
#include "env.h"
    
struct object *eval_expression(struct expression *expr, struct environment *env, struct gc *gc);
struct object *eval_block_statement(struct block_statement *block, struct environment *env, struct gc *gc);

struct object *eval_bang_operator_expression(struct object *obj)
{
    switch (obj->type)
    {
    case OBJ_BOOL:
        return obj == &obj_false ? &obj_true : &obj_false;
        break;
    case OBJ_NULL:
        return &obj_true;
        break;
    default:
        return &obj_false;
        break;
    }
}

struct object *eval_minus_prefix_operator_expression(struct object *right)
{
    if (right->type != OBJ_INT)
    {
        return make_error_object("unknown operator: -%s", object_type_to_str(right->type));
    }

    return make_integer_object(-right->integer);
}

struct object *eval_prefix_expression(operator operator, struct object *right)
{
    switch (operator[0])
    {
    case '!':
        return eval_bang_operator_expression(right);
        break;

    case '-':
        return eval_minus_prefix_operator_expression(right);
        break;

    default: 
        return make_error_object("unknown operator: %s%s", operator, object_type_to_str(right->type));    
    }

    return &obj_null;
}

struct object *eval_integer_infix_expression(operator operator, struct object *left, struct object *right)
{
    struct object *result;

    switch (operator[0])
    {
    case '+':
        result = make_integer_object(left->integer + right->integer);
        break;
    case '-':
        result = make_integer_object(left->integer - right->integer);
        break;
    case '*':
        result = make_integer_object(left->integer * right->integer);
        break;
    case '/':
        result = make_integer_object(left->integer / right->integer);
        break;
    case '<':
        result = make_boolean_object(left->integer < right->integer);
        break;
    case '>':
        result = make_boolean_object(left->integer > right->integer);
        break;
    case '=':
        if (operator[1] == '=') {
            result = make_boolean_object(left->integer == right->integer);
        } else {
            result = &obj_null;
        }
        break;
    case '!':
        if (operator[1] == '=') {
            result = make_boolean_object(left->integer != right->integer);
        } else { 
            result = &obj_null;
        }
        break;
    default:
        result = &obj_null;
        break;
    }

    return result;
}

struct object *eval_infix_expression(operator operator, struct object *left, struct object *right)
{
    if (left->type != right->type) {
        return make_error_object("type mismatch: %s %s %s", object_type_to_str(left->type), operator, object_type_to_str(right->type));
    }

    if (left->type == OBJ_INT && right->type == OBJ_INT)
    {
        return eval_integer_infix_expression(operator, left, right);
    }

    if (left->type == OBJ_BOOL && right->type == OBJ_BOOL)
    {
        if (operator[0] == '=' && operator[1] == '=')
        {
            return make_boolean_object(left == right);
        }

        if (operator[0] == '!' && operator[1] == '=')
        {
            return make_boolean_object(left != right);
        }
    }

    return make_error_object("unknown operator: %s %s %s", object_type_to_str(left->type), operator, object_type_to_str(right->type));
}

unsigned char is_object_truthy(struct object *obj)
{
    if (!obj) {
        return 0;
    }

    if (obj == &obj_null || obj == &obj_false)
    {
        return 0;
    }

    return 1;
}

unsigned char is_object_error(struct object *obj) {
    return obj && obj->type == OBJ_ERROR;
}

struct object *eval_if_expression(struct if_expression *expr, struct environment *env, struct gc *gc)
{
    struct object *obj = eval_expression(expr->condition, env, gc);
    if (is_object_error(obj)) {
        return obj;
    }

    if (is_object_truthy(obj)) {
        return eval_block_statement(expr->consequence, env, gc);
    } else if (expr->alternative) {
        return eval_block_statement(expr->alternative, env, gc);
    }

    return &obj_null;
}

struct object *eval_identifier(struct identifier *ident, struct environment *env) {
    struct object *obj = environment_get(env, ident->value);
    if (obj == NULL) {
        return make_error_object("identifier not found: %s", ident->value);
    }

    return obj;
};

struct object_list *eval_expression_list(struct expression_list *list, struct environment *env, struct gc *gc) {
    struct object_list *result = malloc(sizeof (struct object_list));
    if (!result) {
        errx(EXIT_FAILURE, "out of memory");
    }

    result->values = malloc(sizeof (struct object) * list->size);
    result->size = 0;
    if (!result->values) {
        errx(EXIT_FAILURE, "out of memory");
    }
   
    struct object *obj;
    for (int i = 0; i < list->size; i++) {
        obj = eval_expression(list->values[i], env, gc);
        result->values[result->size++] = obj;

        if (is_object_error(obj)) {
            // TODO: Move obj to start of list
            return result;
        }

        
    }

    return result;
};

struct object *apply_function(struct object *obj, struct object_list *args, struct gc *gc) {
    if (obj->type != OBJ_FUNCTION) {
        return make_error_object("not a function: %s", object_type_to_str(obj->type));
    }

    struct environment *env = make_closed_environment(obj->function.env, 8);
    for (int i=0; i < obj->function.parameters->size; i++) {
        environment_set(env, obj->function.parameters->values[i].value, args->values[i]);
    }

    struct object *result = eval_block_statement(obj->function.body, env, gc);
    free(args->values);
    free(args);
    free_environment(env);

    if (!result) {
        return NULL;
    }
    result->return_value = 0;   
    return result;
};

struct object *eval_expression(struct expression *expr, struct environment *env, struct gc *gc)
{
    struct object *left = NULL;
    struct object *right = NULL;
    struct object *result = NULL;
    struct object_list *args = NULL;

    switch (expr->type)
    {
        case EXPR_INT:
            result = make_integer_object(expr->integer.value);
            gc_add(gc, result);
            return result;
        case EXPR_BOOL:
            return make_boolean_object(expr->bool.value);
        case EXPR_PREFIX:
            right = eval_expression(expr->prefix.right, env, gc);
            if (is_object_error(right)) {
                return right;
            }
            result = eval_prefix_expression(expr->prefix.operator, right);
            gc_add(gc, result);
            return result;
        case EXPR_INFIX:
            left = eval_expression(expr->infix.left, env, gc);
            if (is_object_error(left)) {
                return left;
            }

            right = eval_expression(expr->infix.right, env, gc);
            if (is_object_error(right)) {
                return right;
            }

            result = eval_infix_expression(expr->infix.operator, left, right);
            gc_add(gc, result);
            return result;
        case EXPR_IF:
            result = eval_if_expression(&expr->ifelse, env, gc);
            gc_add(gc, result);
            return result;
        case EXPR_IDENT: 
            result = eval_identifier(&expr->ident, env);    
            //gc_add(gc, result);
            return result;
        case EXPR_FUNCTION: 
            result = make_function_object(&expr->function.parameters, expr->function.body, env);
            gc_add(gc, result);
            return result;
        case EXPR_CALL: 
            left = eval_expression(expr->call.function, env, gc);
            gc_add(gc, left);
            if (is_object_error(result)) {
                return result;
            }

            args = eval_expression_list(expr->call.arguments, env, gc);
            if (args->size >= 1 && is_object_error(args->values[0])) {
                return args->values[0];
            }

            result = apply_function(left, args, gc);
            gc_add(gc, result);
            return result;
    }
    
    return &obj_null;
}

struct object *make_return_object(struct object *obj)
{
    switch (obj->type)
    {
    case OBJ_INT:
    case OBJ_FUNCTION:
        obj->return_value = 1;
        break;
    case OBJ_BOOL:
        if (obj == &obj_false) {
            return &obj_false_return;
        } else {
            return &obj_true_return;
        }
        break;
    case OBJ_NULL:
    case OBJ_ERROR:
        obj = &obj_null_return;
        break;
    }

    return obj;
}

struct object *eval_statement(struct statement *stmt, struct environment *env, struct gc *gc)
{
    struct object *result;

    if (gc->size >= 100) {
        gc_mark_env(gc, env);
        gc_sweep(gc);
    }

    switch (stmt->type)
    {
    case STMT_EXPR:
        result = eval_expression(stmt->value, env, gc);
        return result;
    case STMT_LET: 
        result = eval_expression(stmt->value, env, gc);
        environment_set(env, stmt->name.value, result);
        return result;
    case STMT_RETURN:
        result = eval_expression(stmt->value, env, gc);
        make_return_object(result);
        return result;
    }

    return &obj_null;
};

struct object *eval_block_statement(struct block_statement *block, struct environment *env, struct gc *gc)
{
    if (block->size == 0) {
        return NULL;
    }

    struct object *obj;
    for (int i = 0; i < block->size; i++)
    {
        obj = eval_statement(&block->statements[i], env, gc);
        if (obj->return_value || obj->type == OBJ_ERROR)
        {
            return obj;
        }
    }

    return obj;
}

struct object *eval_program(struct program *prog, struct environment *env)
{
    if (prog->size == 0) {
        return NULL;
    }

    struct gc gc = {.size = 0};
    struct object *obj;

    for (int i = 0; i < prog->size; i++)
    {
        obj = eval_statement(&prog->statements[i], env, &gc);
        if (obj && (obj->return_value || obj->type == OBJ_ERROR))
        {
            return obj;
        }
    }

    return obj;
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
        strcat(str, obj == &obj_true ? "true" : "false");
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
    }
}


#endif