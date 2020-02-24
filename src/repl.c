#include <stdio.h>
#include <stdlib.h>

#include "eval.h"

// TODO: perhaps mock this in case it's not installed, since it's not super necessary
#include <editline/readline.h>

int main(int argc, char **argv)
{
    puts("Monkey-C Version 0.0.1");
    puts("Press Ctrl+c to Exit\n");
    char *output = malloc(1024);
    if (!output) {
        puts("Failed to allocate memory for output buffer");
        exit(1);
    }

    struct lexer lexer;
    struct parser parser;
    struct program *program;
    struct environment *env = make_environment(256);

    while (1)
    {
        char *input = readline("monkey> ");
        add_history(input);

        lexer = new_lexer(input);
        parser = new_parser(&lexer);
        program = parse_program(&parser);

        if (parser.errors > 0) {
            printf("Whoops! Parsing error:\n");
            for (int i = 0; i < parser.errors; i++) {
                printf("- %s\n", parser.error_messages[i]);
            }

            free(input);
            free_program(program);
            continue;
        }

        // evaluate program into buffer
        struct object *obj = eval_program(program, env);
        if (obj->type != OBJ_FUNCTION) {
            object_to_str(output, obj);
            printf("%s\n", output);
        }
       
        // clear output buffer
        output[0] = '\0';

        free(input);
    }

    free_environment(env);
    free(output);
    return 0;
}