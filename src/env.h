#pragma once 

#include "object.h"

#define MAX_KEY_LENGTH 32

struct environment {
    struct object *table[26];
    struct environment *outer;

    // for linking in env_pool
    struct environment *next; 
};

struct environment *make_environment();
struct environment *make_closed_environment(struct environment *parent);
struct object *environment_get(struct environment *env, char *key);
void environment_set(struct environment *env, char *key, struct object *value);
void free_environment(struct environment *env);
void free_env_pool();