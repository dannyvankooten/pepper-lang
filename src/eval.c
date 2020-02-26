
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <err.h>

#include "builtins.h"
#include "object.h"
#include "eval.h"
#include "parser.h"
#include "env.h"

struct object *eval_expression(struct expression *expr, struct environment *env);
struct object *eval_block_statement(struct block_statement *block, struct environment *env);

struct object *eval_bang_operator_expression(struct object *obj)
{
    if (!is_object_truthy(obj)) {
        return object_true;
    }

    return object_false;
}

struct object *eval_minus_prefix_operator_expression(struct object *right)
{
    if (right->type != OBJ_INT)
    {
        return make_error_object("unknown operator: -%s", object_type_to_str(right->type));
    }

    return make_integer_object(-right->integer);
}

struct object *eval_prefix_expression(enum operator operator, struct object *right)
{
    switch (operator)
    {
    case OP_NEGATE:
        return eval_bang_operator_expression(right);
        break;

    case OP_SUBTRACT:
        return eval_minus_prefix_operator_expression(right);
        break;

    default: 
        return make_error_object("unknown operator: %s%s", operator_to_str(operator), object_type_to_str(right->type));    
    }

    return object_null;
}


struct object *eval_integer_infix_expression(enum operator operator, struct object *left, struct object *right)
{
    struct object *result;

    switch (operator)
    {
    case OP_ADD:
        result = make_integer_object(left->integer + right->integer);
        break;
    case OP_SUBTRACT:
        result = make_integer_object(left->integer - right->integer);
        break;
    case OP_MULTIPLY:
        result = make_integer_object(left->integer * right->integer);
        break;
    case OP_DIVIDE:
        result = make_integer_object(left->integer / right->integer);
        break;
    case OP_LT:
        result = make_boolean_object(left->integer < right->integer);
        break;
    case OP_GT:
        result = make_boolean_object(left->integer > right->integer);
        break;
    case OP_EQ:
        result = make_boolean_object(left->integer == right->integer);
        break;
    case OP_NOT_EQ:
        result = make_boolean_object(left->integer != right->integer);
        break;
    default:
        result = object_null;
        break;
    }

    return result;
}

struct object *eval_string_infix_expression(enum operator operator, struct object *left, struct object *right)
{
    if (operator != OP_ADD) {
        return make_error_object("unknown operator: %s %s %s", object_type_to_str(left->type), operator_to_str(operator), object_type_to_str(right->type));
    }

    return make_string_object(left->string, right->string);
}


struct object *eval_infix_expression(enum operator operator, struct object *left, struct object *right)
{
    if (left->type != right->type) 
    {
        return make_error_object("type mismatch: %s %s %s", object_type_to_str(left->type), operator_to_str(operator), object_type_to_str(right->type));
    }

    switch (left->type) {
        case OBJ_INT: 
            return eval_integer_infix_expression(operator, left, right);
        break;

        case OBJ_BOOL:
            if (operator == OP_EQ) {
                return make_boolean_object(left == right);
            } else if (operator == OP_NOT_EQ) {
                return make_boolean_object(left != right);
            }
        break;

        case OBJ_STRING: 
            return eval_string_infix_expression(operator, left, right);
        break;

        default: 
        break;
    }

    return make_error_object("unknown operator: %s %s %s", object_type_to_str(left->type), operator_to_str(operator), object_type_to_str(right->type));
}


struct object *eval_if_expression(struct if_expression *expr, struct environment *env)
{
    struct object *obj = eval_expression(expr->condition, env);
    if (is_object_error(obj->type)) {
        return obj;
    }

    bool truthy = is_object_truthy(obj);
    free_object(obj);

    if (truthy) {
        return eval_block_statement(expr->consequence, env);
    } else if (expr->alternative) {
        return eval_block_statement(expr->alternative, env);
    }

    return object_null;
}


struct object *eval_identifier(struct identifier *ident, struct environment *env) {
    struct object *obj = environment_get(env, ident->value);
    if (obj) {
        return obj;
    }

    obj = get_builtin(ident->value);
    if (obj) {
        return obj;
    }

    return make_error_object("identifier not found: %s", ident->value);
}


struct object_list *eval_expression_list(struct expression_list *list, struct environment *env) {
    struct object_list *result = make_object_list(list->size);
    
    for (int i = 0; i < list->size; i++) {
        struct object *obj = eval_expression(list->values[i], env);
        result->values[result->size++] = obj;

        if (is_object_error(obj->type)) {
            // move object to start of values because that's the only error type we check
            if (result->size > 1) {
                result->values[0] = result->values[result->size-1];

                for (int j=1; j < result->size; j++) {
                    free_object(result->values[j]);
                }
            } 

            return result;
        }
    }

    return result;
}


struct object *apply_function(struct object *obj, struct object_list *args) {

    switch (obj->type) {
        case OBJ_BUILTIN: {
            return obj->builtin(args);
        }
        break;

        case OBJ_FUNCTION: {
            if (args->size != obj->function.parameters->size) {
                return make_error_object("invalid function call: expected %d arguments, got %d", obj->function.parameters->size, args->size);
            }
            
            struct environment *env = make_closed_environment(obj->function.env, 4); 
            for (int i=0; i < obj->function.parameters->size; i++) {
                environment_set(env, obj->function.parameters->values[i].value, args->values[i]);
            }
            struct object *result = eval_block_statement(obj->function.body, env);
            free_environment(env);
            result->return_value = false;
            return result;
        }
        break;

        default: 
            return make_error_object("not a function: %s", object_type_to_str(obj->type));
        break;
    }
  
    return NULL;
}

struct object *eval_index_expression(struct object *left, struct object *index) {
    if (left->type != OBJ_ARRAY || index->type != OBJ_INT) {
        return make_error_object("index operator not supported: %s", object_type_to_str(left->type));
    }

    if (index->integer < 0 || index->integer > (left->array->size - 1)) {
        return object_null;
    }

    return copy_object(left->array->values[index->integer]);
}

struct object *eval_expression(struct expression *expr, struct environment *env)
{
    switch (expr->type)
    {
        case EXPR_INT:
            return make_integer_object(expr->integer);
            break;
        case EXPR_BOOL:
            return make_boolean_object(expr->boolean);
            break;
        case EXPR_STRING: 
            return make_string_object(expr->string, NULL);
            break;
        case EXPR_PREFIX: {
            struct object *right = eval_expression(expr->prefix.right, env);
            if (is_object_error(right->type)) {
                return right;
            }
            struct object *result = eval_prefix_expression(expr->prefix.operator, right);
            free_object(right);
            return result;
            break;
        }
        case EXPR_INFIX: {
            struct object *left = eval_expression(expr->infix.left, env);
            if (is_object_error(left->type)) {
                return left;
            }

            struct object *right = eval_expression(expr->infix.right, env);
            if (is_object_error(right->type)) {
                free_object(left);
                return right;
            }

            struct object *result = eval_infix_expression(expr->infix.operator, left, right);
            free_object(left);
            free_object(right);
            return result;
            break;
        }
        case EXPR_IF:
            return eval_if_expression(&expr->ifelse, env);
            break;
        case EXPR_IDENT: 
            return eval_identifier(&expr->ident, env);
            break;
        case EXPR_FUNCTION: 
            return make_function_object(&expr->function.parameters, expr->function.body, env);
            break;
        case EXPR_CALL: {
            struct object *left = eval_expression(expr->call.function, env);
            if (is_object_error(left->type)) {
                return left;
            }

            struct object_list *args = eval_expression_list(&(expr->call.arguments), env);
            if (args->size >= 1 && is_object_error(args->values[0]->type)) {
                free_object(left);
                return args->values[0];
            }

            struct object *result = apply_function(left, args);
            free_object_list(args);
            free_object(left);
            return result;
            break;
        }

        case EXPR_ARRAY: {
            struct object_list *elements = eval_expression_list(&expr->array, env);
            if (elements->size >= 1 && is_object_error(elements->values[0]->type)) {
                return elements->values[0];
            }
            struct object *result = make_array_object(elements);
            free_object_list(elements);
            return result;
            break;
        }

        case EXPR_INDEX: {
            struct object *left = eval_expression(expr->index.left, env);
            if (is_object_error(left->type)) {
                return left;
            }

            struct object *index = eval_expression(expr->index.index, env);
            if (is_object_error(index->type)) {
                free_object(left);
                return index;
            }

            struct object *result = eval_index_expression(left, index);
            free_object(left);
            free_object(index);
            return result;
        }
    }
    
    return object_null;
}


struct object *make_return_object(struct object *obj)
{
    switch (obj->type)
    {
    case OBJ_INT:
    case OBJ_FUNCTION:
    case OBJ_ERROR:
    case OBJ_STRING:
    case OBJ_ARRAY: {
        obj->return_value = true;
        break;
    } 
       
    case OBJ_BOOL:
        if (obj == object_false) {
            return object_false_return;
        } else {
            return object_true_return;
        }
        break;
    case OBJ_NULL:
        obj = object_null_return;
        break;

    case OBJ_BUILTIN:
        break;    
    }

    return obj;
}


struct object *eval_statement(struct statement *stmt, struct environment *env)
{
    struct object *result = NULL;

    switch (stmt->type)
    {
    case STMT_EXPR:
        result = eval_expression(stmt->value, env);
        return result;
    case STMT_LET: 
        result = eval_expression(stmt->value, env);
        environment_set(env, stmt->name.value, result);
        return result;
    case STMT_RETURN:
        result = eval_expression(stmt->value, env);
        result = make_return_object(result);
        return result;
    }

    return object_null;
}


struct object *eval_block_statement(struct block_statement *block, struct environment *env)
{
    struct object *obj = NULL;
    for (int i = 0; i < block->size; i++)
    {
        if (obj) {
            free_object(obj);
        }

        obj = eval_statement(&block->statements[i], env);
        if (obj->return_value) {
            break;
        }
    }

    // create a fresh copy so we can clear out the environment after the current function goes out of scope
    struct object *copy = copy_object(obj);
    copy->return_value = true;
    free_object(obj);
    return copy;
}

struct object *eval_program(struct program *prog, struct environment *env)
{
    struct object *obj = NULL;

    for (int i = 0; i < prog->size; i++)
    {
        if (obj) {
            free_object(obj);
        }

        obj = eval_statement(&prog->statements[i], env);
        if (obj->return_value) {
           break;
        }
    }

    struct object *copy = copy_object(obj);
    free_object(obj);
    return copy;
}

