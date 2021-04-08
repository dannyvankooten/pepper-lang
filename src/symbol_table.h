#pragma once

#include <inttypes.h>

enum symbol_scope {
    SCOPE_GLOBAL,
    SCOPE_LOCAL,
    SCOPE_FUNCTION,
    SCOPE_BUILTIN,
};

// static const char *symbol_scope_names[] = {
//     "GLOBAL",
//     "LOCAL",
//     "FUNCTION",
//     "BUILTIN",
// };

struct hashmap {
    struct hashmap_node *table[26];
};

struct hashmap_node {
    char *key;
    void *value;
    struct hashmap_node *next;
};

struct symbol {
    char *name;
    enum symbol_scope scope;
    uint32_t index;
};

struct symbol_table {
    uint32_t size;
    struct hashmap *store;
    struct symbol_table *outer;
};

struct symbol_table *symbol_table_new();
struct symbol_table *symbol_table_new_enclosed(struct symbol_table *outer);
struct symbol *symbol_table_define(struct symbol_table *t, char *name);
struct symbol *symbol_table_define_function(struct symbol_table *t, char *name);
struct symbol *symbol_table_define_builtin_function(struct symbol_table *t, uint32_t index, char *name);
struct symbol *symbol_table_resolve(struct symbol_table *t, char *name);
void symbol_table_free(struct symbol_table *t);
