#include "env.h"
#include "test_helpers.h"

int main() {
    struct environment *env = make_environment(32);

    // set
    struct object o1 = {.integer = 1};
    struct object o2 = {.integer = 2};


    environment_set(env, "foo", &o1);
    
    // get
    struct object *r1 = environment_get(env, "foo");
    assertf(r1->integer == o1.integer, "expected %d, got %d", o1.integer, r1->integer);

    // get with other key
    char *k2 = malloc(32);
    strcpy(k2, "foo");
    struct object *r2 = environment_get(env, k2);
    assertf(r2->integer == o1.integer, "expected %d, got %d", o1.integer, r2->integer);

    // update
    environment_set(env, "foo", &o2);
    struct object *r3 = environment_get(env, "foo");
    assertf(r3->integer == o2.integer, "expected %d, got %d", o2.integer, r3->integer);


    // free env
    free(k2);
    free_environment(env);

    printf("\x1b[32mAll env tests passed!\033[0m\n");
}