#ifndef SYMBOL_TABLE_H 
#define SYMBOL_TABLE_H 

enum symbol_scope {
    SCOPE_GLOBAL,
    SCOPE_LOCAL,
};

static const char *symbol_scope_names[] = {
    "GLOBAL",
    "LOCAL",
};

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
    int index;
};

struct symbol_table {
    int size;
    struct hashmap *store;
    struct symbol_table *outer;
};

#define hash(v) (v[0] - 'a') % 26

struct hashmap *hashmap_new();
void hashmap_insert(struct hashmap *hm, char *key, void *item);
void *hashmap_get(struct hashmap *hm, char *key);
void hashmap_free(struct hashmap *hm);

struct symbol_table *symbol_table_new();
struct symbol_table *symbol_table_new_enclosed(struct symbol_table *outer);
struct symbol *symbol_table_define(struct symbol_table *t, char *name);
struct symbol *symbol_table_resolve(struct symbol_table *t, char *name);
void symbol_table_free(struct symbol_table *t);

#endif