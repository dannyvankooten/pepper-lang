#ifndef ENV_H 
#define ENV_H 

#define MAX_KEY_LENGTH 32

#include <string.h> 
#include <stdlib.h> 
#include <err.h>
#include "object.h"

struct env_pool {
    struct environment *head;
};

struct env_pool env_pool = {
    .head = NULL,
};

struct environment {
    struct object **table;
    unsigned int size;
    unsigned int cap;
    struct environment *outer;

    // for linking in env_pool
    struct environment *next; 
};

static unsigned long djb2(char *str)
{
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
    if (!env_pool.head) {
        env = malloc(sizeof *env);
        if (!env) {
            errx(EXIT_FAILURE, "out of memory");
        }
        env->table = malloc(sizeof *env->table * cap);
        if (!env->table) {
            errx(EXIT_FAILURE, "out of memory");
        }

        env->cap = cap;
    } else {
        env = env_pool.head;
        env_pool.head = env->next;

        // increase capacity of existing env if needed
        if (env->cap < cap) {
            env->table = realloc(env->table, sizeof *env->table * cap);
        }
    }
    
    env->size = 0;
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

struct object *environment_get_with_hash(struct environment *env, char *key, unsigned long hash) {
    unsigned int pos = hash % env->cap;
    struct object *node = env->table[pos];

    while (node) {
        if (strncmp(node->name, key, MAX_KEY_LENGTH) == 0) {
            return node;
        }

        node = node->next;
    }

    // try parent environment (bubble up scope)
    if (env->outer) {
        return environment_get_with_hash(env->outer, key, hash);
    }

    return NULL;
}

struct object *environment_get(struct environment *env, char *key) {
   unsigned long hash = djb2(key);
   return environment_get_with_hash(env, key, hash);
}

void environment_set(struct environment *env, char *key, struct object *value) {
    unsigned int pos = djb2(key) % env->cap;
    struct object *list = env->table[pos];
    struct object *node = list;
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

    value->next = list;
    env->table[pos] = value;
    env->size++;
}

void free_environment(struct environment *env) {
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

    // return env to pool so future calls of make_environment can use it
    env->next = env_pool.head;
    env_pool.head = env;
}

#endif