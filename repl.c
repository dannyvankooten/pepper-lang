#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

// TODO: perhaps mock this in case it's not installed, since it's not super necessary
#include <editline/readline.h>

int main(int argc, char **argv)
{
    puts("Monkey-C Version 0.0.1");
    puts("Press Ctrl+c to Exit\n");

    while (1)
    {
        char * input = readline("monkey> ");
        add_history(input);

        struct lexer l = {input, 0};
        struct parser parser = new_parser(&l);
        struct program program = parse_program(&parser);

        if (parser.errors > 0) {
            for (int i = 0; i < parser.errors; i++) {
                printf("\t%s\n", parser.error_messages[i]);
            }

            free(input);
            free_program(&program);
            continue;
        }
        
        printf("%s\n", program_to_str(&program));

        free(input);
        free_program(&program);
    }

    return 0;
}