#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eval.h"
#include "compiler.h"
#include "vm.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_PATCH 1

// TODO: perhaps mock this in case it's not installed, since it's not super necessary
#include <editline/readline.h>

char *read_file(const char *filename);

void print_version() {
    printf("Monkey-C %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

int repl() {
    print_version();
    printf("press CTRL+c to exit\n\n");
    char *output = (char *) malloc(1024);
    if (!output) {
        puts("Failed to allocate memory for output buffer");
        exit(1);
    }
    output[0] = '\0';

    struct lexer lexer;
    struct parser parser;
    struct program *program;

    struct symbol_table *symbol_table = symbol_table_new();
    struct object_list *constants = make_object_list(128);
    
    struct object globals[STACK_SIZE];
    for (int i = 0; i < STACK_SIZE; i++) {
        globals[i] = obj_null;
    }

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

        struct compiler *compiler = compiler_new_with_state(symbol_table, constants);
        int err = compile_program(compiler, program);
        if (err) {
            puts(compiler_error_str(err));
            continue;
        }

        struct bytecode *code = get_bytecode(compiler);
        struct vm *machine = vm_new_with_globals(code, globals);
        err = vm_run(machine);
        if (err) {
            printf("Error executing bytecode: %d\n", err);
            continue;
        }

        struct object obj = vm_stack_last_popped(machine);
        if (obj.type != OBJ_NULL && obj.type != OBJ_BUILTIN && obj.type != OBJ_FUNCTION) {
            object_to_str(output, &obj);
            printf("%s\n", output);
        }
       
        // clear output buffer
        output[0] = '\0';

        // copy globals out of VM so we can re-use it next iteration
        for (int i=0; i < STACK_SIZE; i++) {
            globals[i] = machine->globals[i];
        }

        free(input);
    }

    free(output);
    return 0;
}

int run_script(const char *filename) {
    char *input = read_file(filename);
    struct lexer lexer = new_lexer(input);
    struct parser parser = new_parser(&lexer);
    struct program *program = parse_program(&parser);

    if (parser.errors > 0) {
        for (int i = 0; i < parser.errors; i++) {
            puts(parser.error_messages[i]);
        }

        exit(1);
    }

    struct compiler *compiler = compiler_new();
    int err = compile_program(compiler, program);
    if (err) {
        puts(compiler_error_str(err));
        return EXIT_FAILURE;
    }

    struct bytecode *code = get_bytecode(compiler);
    struct vm *machine = vm_new(code);
    err = vm_run(machine);
    if (err) {
        printf("Error executing bytecode: %d\n", err);
        return EXIT_FAILURE;
    }

    char output[BUFSIZ] = {0};
    struct object obj = vm_stack_last_popped(machine);
    if (obj.type != OBJ_NULL && obj.type != OBJ_BUILTIN && obj.type != OBJ_FUNCTION) {
        object_to_str(output, &obj);
        printf("%s\n", output);
    }

    free_program(program);
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return repl();
    }

    if (strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }

    return run_script(argv[1]);
}

char *read_file(const char *filename) {
    char *input = (char *) malloc(BUFSIZ);
    assert(input != NULL);
    input[0] = '\0';
    size_t size = 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Could not open \"%s\" for reading", filename);
        exit(1);
    }

    size_t read = 0;
    while ( (read = fread(input, sizeof(char), BUFSIZ, f)) > 0) {
        size += read;

        if (read >= BUFSIZ) {
            input = (char*) realloc(input, size + BUFSIZ);
            assert(input != NULL);
        }
    }
    fclose(f);
    return input;
}