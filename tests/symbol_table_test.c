#include "test_helpers.h"
#include "symbol_table.h"

void test_define() {
    TESTNAME(__FUNCTION__);

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

void test_resolve_global() {
    TESTNAME(__FUNCTION__);

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

int main() {
    test_define();
    test_resolve_global();
    printf("\x1b[32mAll symbol table tests passed!\033[0m\n");
}