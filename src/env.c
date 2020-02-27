#include <string.h> 
#include <stdlib.h> 
#include <err.h>

#include "object.h"
#include "env.h"

#define hash(s) (s[0] - 'a')

struct environment *env_pool_head;

struct environment *make_environment(unsigned int cap) {
    struct environment *env;

    // try to get pre-allocated object from pool
    if (!env_pool_head) {
        env = malloc(sizeof *env);
        if (!env) {
            err(EXIT_FAILURE, "out of memory");
        }
        env->cap = 0;
        env->table = NULL;
    } else {
        env = env_pool_head;
        env_pool_head = env->next;
    }

    // increase capacity if needed
    if (env->cap < cap) {
        env->table = realloc(env->table, sizeof *env->table * cap);
        env->cap = cap;
        if (!env->table) {
            err(EXIT_FAILURE, "out of memory");
        }
    }

    env->next = NULL;
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
    return env;
}

struct object *environment_get(struct environment *env, char *key) {
    unsigned int pos = hash(key) % env->cap;
    struct object *node = env->table[pos];

    while (node) {
        if (strcmp(node->name, key) == 0) {
            return node;
        }

        node = node->next;
    }

    // try parent environment (bubble up scope)
    if (env->outer) {
        return environment_get(env->outer, key);
    }

    return NULL;
}

void environment_set(struct environment *env, char *key, struct object *value) {
    unsigned int pos = hash(key) % env->cap;
    struct object *node = env->table[pos];
    struct object *prev = NULL;

    value = copy_object(value);
    value->name = key;

    // find existing node with that key
    while (node) {
        if (strcmp(node->name, key) == 0) {
            if (prev) {
                prev->next = value;
            } else {
                env->table[pos] = value;
            }
            value->next = node->next;

            // free old object
            node->name = NULL;
            free_object(node);
            return;
        }      

        node = node->next;
        prev = node;
    }

    // insert node at head of table
    value->next = env->table[pos];
    env->table[pos] = value;
}

void free_environment(struct environment *env) {
    struct object *node;
    struct object *next;

    // free all objects in env
    for (int i=0; i < env->cap; i++) {
        node = env->table[i];
        while (node) {
            next = node->next;
            node->name = NULL;
            free_object(node);
            node = next;
        }
    }

    // return env to pool
    env->next = env_pool_head;
    env_pool_head = env;
}


void free_env_pool() {
    struct environment *node = env_pool_head;
    struct environment *next = NULL;

    while (node) {
        next = node->next;
        free(node->table);
        free(node);
        node = next;
    }

    env_pool_head = NULL;
}