#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "compiler.h"
#include "eval.h"

// TODO: perhaps mock this in case it's not installed, since it's not super necessary
#include <editline/readline.h>
void with_vm() ;
void with_interpreter();

int main(int argc, char **argv)
{
    with_vm();
    return 0;
}

void with_vm() {
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

    struct symbol_table *symbol_table = symbol_table_new();
    struct object_list *constants = make_object_list(128);
    struct object_list *globals = make_object_list(128);

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

        struct compiler *c = compiler_new_with_state(symbol_table, constants);
        int err = compile_program(c, program);
        if (err) {
            puts(compiler_error_str(err));
            continue;
        }

        struct bytecode *code = get_bytecode(c);
        struct vm *machine = vm_new_with_globals(code, globals);
        err = vm_run(machine);
        if (err) {
            printf("Error executing bytecode: %d\n", err);
            continue;
        }

        struct object *obj = vm_stack_last_popped(machine);
        if (obj->type != OBJ_BUILTIN && obj->type != OBJ_FUNCTION) {
            object_to_str(output, obj);
            printf("%s\n", output);
        }
       
        // clear output buffer
        output[0] = '\0';

        free(input);
    }

    free(output);
}

void with_interpreter() {
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
    struct environment *env = make_environment(26);

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
        if (obj->type != OBJ_BUILTIN && obj->type != OBJ_FUNCTION) {
            object_to_str(output, obj);
            printf("%s\n", output);
        }
       
        // clear output buffer
        output[0] = '\0';

        free(input);
    }

    free_environment(env);
    free(output);
}