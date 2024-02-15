#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol_table.h"

#define hash(v) (v[0] - 'a')

static struct hashmap *hashmap_new(void);
static void hashmap_insert(struct hashmap *hm, const char *key, void *item);
static void *hashmap_get(struct hashmap *hm, const char *key);
static void hashmap_free(struct hashmap *hm);

static struct hashmap *
hashmap_new(void) {
    struct hashmap *hm = malloc(sizeof *hm);
    if (!hm) err(EXIT_FAILURE, "out of memory");

    for (unsigned i=0; i < 26u; i++) {
        hm->table[i] = NULL;
    }

    return hm;
}

static void 
hashmap_insert(struct hashmap *hm, const char *key, void *item) {
    const int8_t pos = hash(key);
    struct hashmap_node *head = hm->table[pos];
    struct hashmap_node *node = head;

    // walk list to find existing value with that key
    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->value = item;
            return;
        }

        node = node->next;
    }

    // insert new node at start of list
    node = malloc(sizeof *node);
    assert(node != NULL);
    node->key = key;
    node->value = item;
    node->next = head;
    hm->table[pos] = node;
}

static void *
hashmap_get(struct hashmap *hm, const char *key) {
    unsigned pos = hash(key);
    struct hashmap_node *node = hm->table[pos];

    while (node) {
         if (strcmp(node->key, key) == 0) {
           return node->value;
        }
        node = node->next;
    }
    return NULL;
}

static 
void hashmap_free(struct hashmap *hm) {
    struct hashmap_node *node;
    struct hashmap_node *next;

    for (unsigned i=0; i < 26u; i++) {
        node = hm->table[i];
        while (node) {
            next = node->next;
            free(node->value);
            free(node);            
            node = next;
        }
    }

    free(hm);
}

struct symbol_table *symbol_table_new(void) {
    struct symbol_table *t = malloc(sizeof *t);
    if (!t) err(EXIT_FAILURE, "out of memory");
    t->size = 0;
    t->store = hashmap_new();
    t->outer = NULL;
    return t;
}

struct symbol_table *symbol_table_new_enclosed(struct symbol_table *outer) {
    struct symbol_table *t = symbol_table_new();
    t->outer = outer;
    return t;
}

struct symbol *symbol_table_define(struct symbol_table *t, const char *name) {
    struct symbol *s;

    // // resolve symbol, but only in the current scope (don't traverse t->outer)
    s = (struct symbol*) hashmap_get(t->store, name);
    if (s != NULL) {
        return s;
    }

    // Note that we're not copying the contents of the name pointer here
    // This means the AST can't be free'd as long as this symbol table in use
    s = malloc(sizeof *s);
    if (!s) err(EXIT_FAILURE, "out of memory");
    s->name = name;
    s->index = t->size;
    s->scope = t->outer ? SCOPE_LOCAL : SCOPE_GLOBAL;

    hashmap_insert(t->store, name, s);
    t->size++;
    return s;
}

struct symbol *symbol_table_define_builtin_function(struct symbol_table *t, uint32_t index, const char *name) {
    struct symbol *s = malloc(sizeof *s);
    if (!s) err(EXIT_FAILURE, "out of memory");

    // Note that we're not copying the contents of the name pointer here
    // This means the AST can't be free'd as long as this symbol table in use
    s->name = name;
    s->scope = SCOPE_BUILTIN;
    s->index = index;
    hashmap_insert(t->store, name, s);
    return s;    
}

struct symbol *symbol_table_resolve(struct symbol_table *t, const char *name) {
    void *r = hashmap_get(t->store, name);
    if (r == NULL) {
        if (t->outer) {
            return symbol_table_resolve(t->outer, name);
        }

        return NULL;
    }
    
    return (struct symbol *) r;
}

void symbol_table_free(struct symbol_table *t) {
    hashmap_free(t->store);
    free(t);
}
