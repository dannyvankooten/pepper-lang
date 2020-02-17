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

int main() {
    char * input = ""
        "let x = 5;\n"
        "let y = 10;\n"
        "let foobar = 838383;\n";

    lexer l = {
        .input = input,
        .pos = 0
    };

    parser parser = new_parser(&l);
    program p = parse_program(&parser);
   
    if (p.size != 3) {
        abortf("wrong program size. expected %d, got %d\n", 3, p.size);
    }    

    char tests[3][100] = {
        "x", 
        "y",
        "foobar",
    };
    for (int i = 0; i < 3; i++) {
        if (strcmp(p.statements[i]->token.literal, "let") != 0) {
            abortf("wrong literal. expected %s, got %s\n", "let", p.statements[i]->token.literal);
        }

        if (strcmp(p.statements[i]->name.value, tests[i]) != 0) {
            abortf("wrong name value. expected %s, got %s\n", tests[i], p.statements[i]->name.value);
        }

        // if (strcmp(p.statements[i].name.token.literal, tests[i]) != 0) {
        //     abortf("wrong name literal. expected %s, got %s", tests[i], p.statements[i].token.literal);
        // }
    }

}

