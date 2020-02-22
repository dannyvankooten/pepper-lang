#ifndef ENV_H 
#define ENV_H 

#define MAX_KEY_LENGTH 32

#include <string.h> 
#include <stdlib.h> 
#include <err.h>
#include "object.h"

struct node {
    char *key;
    struct object *value;
    struct node *next;
};

struct environment {
    struct node **table;
    unsigned int size;
    unsigned int cap;
    struct environment *outer;
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
    struct environment *env = malloc(sizeof(struct environment));
    env->cap = cap;
    env->size = 0;
    env->table = malloc(sizeof(struct node) * cap);
    if (!env->table) {
        errx(EXIT_FAILURE, "out of memory");
    }
    
    env->outer = NULL;
    for (int i = 0; i < env->cap; i++)
    {
        env->table[i] = NULL;
    }
    return env;
};

struct environment *make_closed_environment(struct environment *parent, unsigned int cap) {
    struct environment *env = make_environment(cap);
    env->outer = parent;
    return env;
};

struct object *environment_get_with_hash(struct environment *env, char *key, unsigned long hash) {
    unsigned int pos = hash % env->cap;
    struct node *node = env->table[pos];

    while (node) {
        if (strncmp(node->key, key, MAX_KEY_LENGTH) == 0) {
            return node->value;
        }

        node = node->next;
    }

    // try parent environment (bubble up scope)
    if (env->outer) {
        return environment_get_with_hash(env->outer, key, hash);
    }

    return NULL;
};

struct object *environment_get(struct environment *env, char *key) {
   unsigned long hash = djb2(key);
   return environment_get_with_hash(env, key, hash);
};

void environment_set(struct environment *env, char *key, struct object *value) {
    unsigned int pos = djb2(key) % env->cap;
    struct node *list = env->table[pos];
    struct node *node = list;

    // find existing node with that key
    while (node) {
        if (strncmp(node->key, key, MAX_KEY_LENGTH) == 0) {
            free_object(node->value);
            node->value = value;
            return;
        }      

        node = node->next;
    }

    // add new node to start of list
    node = malloc(sizeof (struct node));
    if (!node) {
        errx(EXIT_FAILURE, "out of memory");
    }
    node->next = list;
    node->key = malloc(strlen(key) + 1);
    if (!node->key) {
        errx(EXIT_FAILURE, "out of memory");
    }
    strcpy(node->key, key);
    node->value = value;
    env->table[pos] = node;
    env->size++;
};

void free_environment(struct environment *env) {
    if (!env) {
        return;
    }

    struct node *node;
    struct node *next;

    for (int i=0; i < env->cap; i++) {
        node = env->table[i];

        while (node) {
            next = node->next;
            free(node->key);
            free_object(node->value);
            free(node);
            node = next;
        }
    }

    free(env->table);
    free(env);
}

#endif