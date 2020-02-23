#include "env.h"
#include "test_helpers.h"

int main() {
    struct environment *env = make_environment(32);

    // set
    struct object o1 = {.integer = 1 };
    struct object o2 = {.integer = 2};

    environment_set(env, "foo", &o1);
    environment_set(env, "bar", &o2);
    
    // get
    struct object *r1 = environment_get(env, "foo");
    assertf(r1->integer == o1.integer, "expected %d, got %d", o1.integer, r1->integer);
    struct object *r2 = environment_get(env, "bar");
    assertf(r2->integer == o2.integer, "expected %d, got %d", o2.integer, r2->integer);

    // not existing
    assertf(environment_get(env, "unexisting") == NULL, "expected NULL, got something");

    // free env
    free_environment(env);

    printf("\x1b[32mAll env tests passed!\033[0m\n");
}