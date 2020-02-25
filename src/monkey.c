#include <stdio.h>
#include <stdlib.h>

#include "eval.h"

char *read_file(char *filename);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: %s file.monkey", argv[0]);
        exit(1);
    }

    char *input = read_file(argv[1]);
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);

    if (parser.errors > 0) {
        for (int i = 0; i < parser.errors; i++) {
            puts(parser.error_messages[i]);
        }

        exit(1);
    }

    struct environment *env = make_environment(26);
    struct object *obj = eval_program(program, env);
    char output[256];
    output[0] = '\0';
    object_to_str(output, obj);
    puts(output);

    free_program(program);
    free_environment(env);
}

char *read_file(char *filename) {
    char *input = malloc(1024);
    unsigned int size = 0;

    FILE *f = fopen(filename, "r");
    unsigned int read = 0;
    while ( (read = fread(input, 512, 1024, f)) > 0) {
        size += read;
        input = realloc(input, size + 1024);
    }
    fclose(f);
    return input;
}