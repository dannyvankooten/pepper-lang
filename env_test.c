#include "env.h"
#include "test_helpers.h"

int main() {
    struct environment *env = make_environment(32);

    // set
    environment_set(env, "foo", "bar");
    
    // get
    char *v = environment_get(env, "foo");
    assertf(strcmp(v, "bar") == 0, "expected %s, got %s", "bar", v);

    // get with other key
    char *k2 = malloc(32);
    strcpy(k2, "foo");
    assertf(strcmp(environment_get(env, k2), "bar") == 0, "expected %s, got %s", "bar", v);

    // update
    environment_set(env, "foo", "bar-bar");
    char *v2 = environment_get(env, "foo");
    assertf(strcmp(v2, "bar-bar") == 0, "expected %s, got %s", "bar-bar", v);

    // free env
    free(k2);
    free_environment(env);

    printf("\x1b[32mAll env tests passed!\033[0m\n");
}