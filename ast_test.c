#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include "ast.h"
#include <stdarg.h> 

void abortf(char *format, ...) {
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);

   abort();
}

void assert_parser_errors(parser *p) {
    if (p->errors > 0) {
        for (int i = 0; i < p->errors; i++) {
            printf("parser error: %s\n", p->error_messages[i]);
        }

        abort();
    }
}

void assert_program_size(program *p, unsigned expected_size) {
    if (p->size != expected_size) {
        abortf("wrong program size. expected %d, got %d\n", expected_size, p->size);
    }   
}

void test_let_statements() {
    char * input = ""
        "let x = 5;\n"
        "let y = 10;\n"
        "let foo = 838383;\n";

    lexer l = {input, 0 };
    parser parser = new_parser(&l);
    program p = parse_program(&parser);
    
    assert_parser_errors(&parser);
    assert_program_size(&p, 3);    

    struct test {
        char * literal;
        char * name;
    } tests[3] = {
        {"let", "x"},
        {"let", "y"},
        {"let", "foo"}
    };

    for (int i = 0; i < 3; i++) {
        if (strcmp(p.statements[i].token.literal, tests[i].literal) != 0) {
            abortf("wrong literal. expected %s, got %s\n", tests[i].literal, p.statements[i].token.literal);
        }

        if (strcmp(p.statements[i].name.value, tests[i].name) != 0) {
            abortf("wrong name value. expected %s, got %s\n", tests[i].name, p.statements[i].name.value);
        }

        if (strcmp(p.statements[i].name.token.literal, tests[i].name) != 0) {
            abortf("wrong name literal. expected %s, got %s", tests[i].name, p.statements[i].token.literal);
        }
    }
}


void test_return_statements() {
    char * input = ""
        "return 5;\n"
        "return 10;\n"
        "return 993322;\n";

    lexer l = {input, 0 };
    parser parser = new_parser(&l);
    program p = parse_program(&parser);
    
    assert_parser_errors(&parser);
    assert_program_size(&p, 3);   

    struct test {
        char * literal;
        char * name;
    } tests[3] = {
        {"return", ""},
        {"return", ""},
        {"return", ""}
    };

    for (int i = 0; i < 3; i++) {
        if (strcmp(p.statements[i].token.literal, tests[i].literal) != 0) {
            abortf("wrong literal. expected %s, got %s\n", tests[i].literal, p.statements[i].token.literal);
        }

        // TODO: Test expression too
    }
}


int main() {
    test_let_statements();
    test_return_statements();
}
