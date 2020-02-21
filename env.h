#include <string.h> 
#include <stdlib.h> 
#include <err.h>

struct node {
    char *key;
    void *value;
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
    struct environment *env = (struct environment*) malloc(sizeof(struct environment));
    env->cap = cap;
    env->size = 0;
    env->table = (struct node **) malloc(sizeof(struct node) * cap);
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

void *environment_get(struct environment *env, char *key) {
    unsigned int pos = djb2(key) % env->cap;
    struct node *node = env->table[pos];

    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }

        node = node->next;
    }

    // try parent environment (bubble up scope)
    if (env->outer) {
        return environment_get(env->outer, key);
    }

    return NULL;
};

void environment_set(struct environment *env, char *key, void *value) {
    unsigned int pos = djb2(key) % env->cap;
    struct node *list = env->table[pos];
    struct node *node = list;

    // find existing node with that key
    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->value = value;
            return;
        }      

        node = node->next;
    }

    // add new node to start of list
    node = (struct node *) malloc(sizeof (struct node));
    node->next = list;
    node->key = (char *) malloc(strlen(key) + 1);
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

    for (int i=0; i < env->size; i++) {
        node = env->table[i];
        if (!node) {
            continue;
        }

        while (node) {
            next = node->next;
            free(node->key);
            free(node);
            node = next;
        }
    }

    if (env->outer) {
        free_environment(env->outer);
    }

    if (env->table) {
        free(env->table);
    }

    if (env) {
        free(env);
    }
}

