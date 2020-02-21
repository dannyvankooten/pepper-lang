#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "parser.h"
#include "env.h"

enum object_type
{
    OBJ_NULL,
    OBJ_BOOL,
    OBJ_INT,
    OBJ_ERROR,
    OBJ_FUNCTION,
};

static const char *object_names[] = {
    "NULL",
    "BOOLEAN",
    "INTEGER",
    "ERROR",
    "FUNCTION",
};

struct function {
    struct identifier_list *parameters;
    struct block_statement *body;
    struct environment *env;
};

struct object
{
    enum object_type type;
    union {
        unsigned char boolean;
        long integer;
        char *error;
        struct function function;
    };
    unsigned char return_value;
};

struct object_list {
    struct object **values;
    unsigned int size;
    unsigned int cap;
};

struct object obj_null = {
    .type = OBJ_NULL,
    .return_value = 0
};
struct object obj_null_return = {
    .type = OBJ_NULL,
    .return_value = 1
};
struct object obj_true = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 0
};
struct object obj_false = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 0
};
struct object obj_true_return = {
    .type = OBJ_BOOL,
    .boolean = 1,
    .return_value = 1
};
struct object obj_false_return = {
    .type = OBJ_BOOL,
    .boolean = 0,
    .return_value = 1
};

const char *object_type_to_str(enum object_type t);
struct object *eval_expression(struct expression *expr, struct environment *env);
struct object *eval_block_statement(struct block_statement *block, struct environment *env);
void free_object(struct object *obj);

struct object *make_boolean_object(char value)
{
    return value ? &obj_true : &obj_false;
}

struct object *make_integer_object(long value)
{
    struct object *result = malloc(sizeof(struct object));
    result->type = OBJ_INT;
    result->integer = value;
    result->return_value = 0;
    return result;
}

struct object *make_error_object(struct object *obj, char *format, ...) {
    va_list args;

    // try to re-use discarded object here
    if (!obj || obj->type == OBJ_BOOL || obj->type == OBJ_NULL) {
        obj = malloc(sizeof(struct object));
        obj->error = malloc(256);
    } else if (obj->type == OBJ_INT) {
        obj->error = malloc(256);
    }

    obj->type = OBJ_ERROR;
    obj->return_value = 0;
    va_start(args, format);  
    vsnprintf(obj->error, 256, format, args);
    va_end(args);
    return obj;
};

struct object *make_function_object(struct identifier_list *parameters, struct block_statement *body, struct environment *env) {
    struct object *obj = malloc(sizeof(struct object));
    obj->type = OBJ_FUNCTION;
    obj->return_value = 0;
    obj->function.parameters = parameters;
    obj->function.body = body;
    obj->function.env = env;
    return obj;
}

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
        return make_error_object(right, "unknown operator: -%s", object_type_to_str(right->type));
    }

    right->integer = -right->integer;
    return right;
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
        return make_error_object(right, "unknown operator: %s%s", operator, object_type_to_str(right->type));    
    }

    return &obj_null;
}

struct object *eval_integer_infix_expression(operator operator, struct object *left, struct object *right)
{
    switch (operator[0])
    {
    case '+':
        return make_integer_object(left->integer + right->integer);
    case '-':
        return make_integer_object(left->integer - right->integer);
    case '*':
        return make_integer_object(left->integer * right->integer);
    case '/':
        return make_integer_object(left->integer / right->integer);
    case '<':
        return make_boolean_object(left->integer < right->integer);
    case '>':
        return make_boolean_object(left->integer > right->integer);
    case '=':
        if (operator[1] == '=')
        {
            return make_boolean_object(left->integer == right->integer);
        }
        return &obj_null;
    case '!':
        if (operator[1] == '=')
        {
            return make_boolean_object(left->integer != right->integer);
        }
        return &obj_null;
    default:
        return &obj_null;
        break;
    }
}

struct object *eval_infix_expression(operator operator, struct object *left, struct object *right)
{
    if (left->type != right->type) {
        return make_error_object(left, "type mismatch: %s %s %s", object_type_to_str(left->type), operator, object_type_to_str(right->type));
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

    return make_error_object(left, "unknown operator: %s %s %s", object_type_to_str(left->type), operator, object_type_to_str(right->type));
}

unsigned char is_object_truthy(struct object *obj)
{
    if (obj == &obj_null || obj == &obj_false)
    {
        return 0;
    }

    return 1;
}

unsigned char is_object_error(struct object *obj) {
    return obj->type == OBJ_ERROR;
}

struct object *eval_if_expression(struct if_expression *expr, struct environment *env)
{
    struct object *obj = eval_expression(expr->condition, env);
    if (is_object_error(obj)) {
        return obj;
    }

    if (is_object_truthy(obj))
    {
        return eval_block_statement(expr->consequence, env);
    }
    else if (expr->alternative)
    {
        return eval_block_statement(expr->alternative, env);
    }

    return &obj_null;
}

struct object *eval_identifier(struct identifier *ident, struct environment *env) {
    void *value = environment_get(env, ident->value);
    if (value == NULL) {
        return make_error_object(NULL, "identifier not found: %s", ident->value);
    }

    return (struct object *) value;;
};

struct object_list *eval_expression_list(struct expression_list *list, struct environment *env) {
    struct object_list *result = malloc(sizeof (struct object_list));
    if (!result) {
        return result;
    }

    result->values = malloc(sizeof (struct object) * list->size);
    result->size = 0;

    struct object *obj;
    for (int i = 0; i < list->size; i++) {
        obj = eval_expression(&list->values[i], env);
        result->values[result->size++] = obj;

        if (is_object_error(obj)) {
            return result;
        }
    }

    return result;
};

struct object *apply_function(struct object *obj, struct object_list *args) {
    if (obj->type != OBJ_FUNCTION) {
        return make_error_object(NULL, "not a function: %s", object_type_to_str(obj->type));
    }

    struct environment *env = make_closed_environment(obj->function.env, 16);
    for (int i=0; i < obj->function.parameters->size; i++) {
        environment_set(env, obj->function.parameters->values[i].value, args->values[i]);
    }

    // TODO: Not sure if necessary to reset return value here
    struct object *result = eval_block_statement(obj->function.body, env);
    result->return_value = 0;
    return result;
};

struct object *eval_expression(struct expression *expr, struct environment *env)
{
    struct object *left = NULL;
    struct object *right = NULL;
    struct object *result = NULL;
    struct object_list *args = NULL;

    switch (expr->type)
    {
        case EXPR_INT:
            return make_integer_object(expr->integer.value);
        case EXPR_BOOL:
            return make_boolean_object(expr->bool.value);
        case EXPR_PREFIX:
            right = eval_expression(expr->prefix.right, env);
            if (is_object_error(right)) {
                return right;
            }
            result = eval_prefix_expression(expr->prefix.operator, right);
            //free_object(right);
            return result;
        case EXPR_INFIX:
            left = eval_expression(expr->infix.left, env);
            if (is_object_error(left)) {
                return left;
            }

            right = eval_expression(expr->infix.right, env);
            if (is_object_error(right)) {
                return right;
            }

            result = eval_infix_expression(expr->infix.operator, left, right);
            //free_object(left);
            //free_object(right);
            return result;
        case EXPR_IF:
            return eval_if_expression(&expr->ifelse, env);
        case EXPR_IDENT: 
            return eval_identifier(&expr->ident, env);    
        case EXPR_FUNCTION: 
            return make_function_object(&expr->function.parameters, expr->function.body, env);
        case EXPR_CALL: 
            result = eval_expression(expr->call.function, env);
            if (is_object_error(result)) {
                return result;
            }

            args = eval_expression_list(&expr->call.arguments, env);
            if (args->size == 1 && is_object_error(args->values[0])) {
                return args->values[0];
            }

            return apply_function(result, args);
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
        if (obj == &obj_false)
        {
            obj = &obj_false_return;
        }
        else
        {
            obj = &obj_true_return;
        }
        break;
    case OBJ_NULL:
    case OBJ_ERROR:
        obj = &obj_null_return;
        break;
    }

    return obj;
}

struct object *eval_statement(struct statement *stmt, struct environment *env)
{
    struct object *result;

    switch (stmt->type)
    {
    case STMT_EXPR:
        return eval_expression(stmt->value, env);
    case STMT_LET: 
        result = eval_expression(stmt->value, env);
        environment_set(env, stmt->name.value, result);
        return result;
    case STMT_RETURN:
        result = eval_expression(stmt->value, env);
        make_return_object(result);
        return result;
    }

    return &obj_null;
};

struct object *eval_block_statement(struct block_statement *block, struct environment *env)
{
    struct object *obj;

    for (int i = 0; i < block->size; i++)
    {
        obj = eval_statement(&block->statements[i], env);
        if (obj->return_value || obj->type == OBJ_ERROR)
        {
            return obj;
        }
    }

    return obj;
}

struct object *eval_program(struct program *prog, struct environment *env)
{
    struct object *obj;

    for (int i = 0; i < prog->size; i++)
    {
        obj = eval_statement(&prog->statements[i], env);
        if (obj->return_value || obj->type == OBJ_ERROR)
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

const char *object_type_to_str(enum object_type t)
{
    return object_names[t] ?: "UNKOWN";
}

void free_object(struct object *obj)
{   
    if (!obj) {
        return;
    }

    if (obj->type == OBJ_ERROR && obj->error) {
        free(obj->error);
    }

    if (obj->type == OBJ_INT || obj->type == OBJ_ERROR)
    {
        free(obj);
    }

}