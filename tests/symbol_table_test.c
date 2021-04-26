#include "test_helpers.h"
#include "../src/symbol_table.h"

static void define() {
    struct {
        char *name;
        struct symbol expected;
    } tests[] = {
        { .name = "a", .expected = { .name = "a", .scope = SCOPE_GLOBAL, .index = 0 } },
        { .name = "b", .expected = { .name = "b", .scope = SCOPE_GLOBAL, .index = 1 } },
    };

    struct symbol_table *global = symbol_table_new();
    
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct symbol *s = symbol_table_define(global, tests[t].name);
        assertf(s != NULL, "expected symbol, got NULL");
        assertf(strcmp(s->name, tests[t].expected.name) == 0, "wrong name: expected %s, got %s", tests[t].expected.name, s->name);
        assertf(s->index == tests[t].expected.index, "wrong index: expected %d, got %d", tests[t].expected.index, s->index);
        assertf(s->scope == tests[t].expected.scope, "wrong scope: expected %d, got %d", tests[t].expected.scope, s->scope);
    }

    symbol_table_free(global);
}

static void resolve_global() {

    struct symbol_table *global = symbol_table_new();
    symbol_table_define(global, "a");
    symbol_table_define(global, "b");

    struct symbol tests[] = {
        { .name = "a", .scope = SCOPE_GLOBAL, .index = 0 },
        { .name = "b", .scope = SCOPE_GLOBAL, .index = 1 },
    };

     for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct symbol *s = symbol_table_resolve(global, tests[t].name);
        assertf(s != NULL, "expected symbol, got NULL");
        assertf(strcmp(s->name, tests[t].name) == 0, "wrong name: expected %s, got %s", tests[t].name, s->name);
        assertf(s->index == tests[t].index, "wrong index: expected %d, got %d", tests[t].index, s->index);
        assertf(s->scope == tests[t].scope, "wrong scope: expected %d, got %d", tests[t].scope, s->scope);
    }

    symbol_table_free(global);
}


static void resolve_local() {

    struct symbol_table *global = symbol_table_new();
    symbol_table_define(global, "a");
    symbol_table_define(global, "b");

    struct symbol_table *local = symbol_table_new_enclosed(global);
    symbol_table_define(local, "c");
    symbol_table_define(local, "d");

    struct symbol tests[] = {
        { .name = "a", .scope = SCOPE_GLOBAL, .index = 0 },
        { .name = "b", .scope = SCOPE_GLOBAL, .index = 1 },
        { .name = "c", .scope = SCOPE_LOCAL, .index = 0 },
        { .name = "d", .scope = SCOPE_LOCAL, .index = 1 },
    };

    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        struct symbol *s = symbol_table_resolve(local, tests[t].name);
        assertf(s != NULL, "expected symbol, got NULL");
        assertf(strcmp(s->name, tests[t].name) == 0, "wrong name: expected %s, got %s", tests[t].name, s->name);
        assertf(s->index == tests[t].index, "wrong index: expected %d, got %d", tests[t].index, s->index);
        assertf(s->scope == tests[t].scope, "wrong scope: expected %d, got %d", tests[t].scope, s->scope);
    }

    symbol_table_free(global);
    symbol_table_free(local);
}

static void define_and_resolve_builtins() {
    struct symbol_table *global = symbol_table_new();
    struct symbol_table *local1 = symbol_table_new_enclosed(global);
    struct symbol_table *local2 = symbol_table_new_enclosed(local1);
    struct symbol tests[] = {
        { .name = "a", .scope = SCOPE_BUILTIN, .index = 0 },
        { .name = "b", .scope = SCOPE_BUILTIN, .index = 1 },
        { .name = "c", .scope = SCOPE_BUILTIN, .index = 2 },
        { .name = "d", .scope = SCOPE_BUILTIN, .index = 3 },
    };
    for (int t=0; t < ARRAY_SIZE(tests); t++) {
        symbol_table_define_builtin_function(global, tests[t].index, tests[t].name);
    }

    // in all scopes, the declared symbols should resolve to our builtin
    struct symbol_table *tables[] = {global, local1, local2};
    for (int i=0; i < 2; i++) {
        for (int t=0; t < ARRAY_SIZE(tests); t++) {
            struct symbol *s = symbol_table_resolve(tables[i], tests[t].name);
            assertf(s != NULL, "expected symbol, got NULL");
            assertf(strcmp(s->name, tests[t].name) == 0, "wrong name: expected %s, got %s", tests[t].name, s->name);
            assertf(s->index == tests[t].index, "wrong index: expected %d, got %d", tests[t].index, s->index);
            assertf(s->scope == tests[t].scope, "wrong scope: expected %d, got %d", tests[t].scope, s->scope);
        }
    }

    symbol_table_free(global);
    symbol_table_free(local1);
    symbol_table_free(local2);
}

int main(int argc, char *argv[]) {
    TEST(define);
    TEST(resolve_global);
    TEST(resolve_local);
    TEST(define_and_resolve_builtins);
}