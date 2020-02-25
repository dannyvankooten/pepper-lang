#include <string.h> 
#include <stdlib.h> 
#include <err.h>

#include "object.h"
#include "env.h"

struct environment *free_env_list;

unsigned long djb2(char *str)
{
    return str[0] - 'a';
    unsigned long hash = 5381;
    int c;

    // hash * 33 + c
    // shifting bits for performance
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; 
    }

    return hash;
}

struct environment *make_environment(unsigned int cap) {
    struct environment *env;

    // try to get pre-allocated object from pool
    if (!free_env_list) {
        env = malloc(sizeof *env);
        if (!env) {
            err(EXIT_FAILURE, "out of memory");
        }
        env->cap = 0;
        env->table = NULL;
    } else {
        env = free_env_list;
        free_env_list = env->next;
    }

    // increase capacity if needed
    if (env->cap < cap) {
        env->table = realloc(env->table, sizeof *env->table * cap);
        env->cap = cap;
        if (!env->table) {
            err(EXIT_FAILURE, "out of memory");
        }
    }

    env->ref_count = 1;
    env->outer = NULL;
    for (int i = 0; i < env->cap; i++)
    {
        env->table[i] = NULL;
    }
    return env;
}

struct environment *make_closed_environment(struct environment *parent, unsigned int cap) {
    struct environment *env = make_environment(cap);
    env->outer = parent;
    env->outer->ref_count++;
    return env;
}

struct object *environment_get_with_pos(struct environment *env, char *key, unsigned int pos) {
    struct object *node = env->table[pos];

    while (node) {
        if (strncmp(node->name, key, MAX_KEY_LENGTH) == 0) {
            return node;
        }

        node = node->next;
    }

    // try parent environment (bubble up scope)
    if (env->outer) {
        return environment_get_with_pos(env->outer, key, pos);
    }

    return NULL;
}

struct object *environment_get(struct environment *env, char *key) {
   unsigned int pos = djb2(key) % env->cap;
   return environment_get_with_pos(env, key, pos);
}

void environment_set(struct environment *env, char *key, struct object *value) {
    unsigned int pos = djb2(key) % env->cap;
    struct object *node = env->table[pos];
    struct object *prev = NULL;

    value->name = key;

    // find existing node with that key
    while (node) {
        if (strncmp(node->name, key, MAX_KEY_LENGTH) == 0) {
            if (prev) {
                prev->next = value;
            } else {
                env->table[pos] = value;
            }
            value->next = node->next;
            free_object(node);
            return;
        }      

        node = node->next;
        prev = node;
    }

    value->next = env->table[pos];
    env->table[pos] = value;
}

void free_environment(struct environment *env) {
    if (--env->ref_count > 0) {
        return;
    }

    if (env->outer) {
        free_environment(env->outer);
    }

    struct object *node;
    struct object *next;

    // free objects in env
    for (int i=0; i < env->cap; i++) {
        node = env->table[i];
        while (node) {
            next = node->next;
            free_object(node);
            node = next;
        }
    }

    env->next = free_env_list;
    free_env_list = env;
}
