#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

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

        lexer l = {
            .input = input,
            .pos = 0
        };
        token t;
        while (gettoken(&l, &t) != -1)
        {
            printf("Type: %s\t Literal: %s\n", t.type, t.literal);
        }

        free(input);
    }

    return 0;
}