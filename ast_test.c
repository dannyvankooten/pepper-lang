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
        printf("parser has %d errors: \n", p->errors);
        for (int i = 0; i < p->errors; i++) {
            printf("  - %s\n", p->error_messages[i]);
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

void test_program_string() {
    program p = {
        .statements = {
            {
                .token = {
                    .type = LET,
                    .literal = "let",
                },
                .name = {
                    .token = {
                        .type = IDENT,
                        .literal = "myVar",
                    },
                    .value = "myVar",
                },
                .value = {
                    .token = {
                        .type = IDENT,
                        .literal = "anotherVar",
                    },
                    .value = "anotherVar",
                }
            }, 
            {
                .token = {
                    .type = RETURN,
                    .literal = "return",
                },
                .value = {
                    .token = {
                        .type = INT,
                        .literal = "5",
                    },
                    .value = "5",
                }
            }, 
        }, 
        .size = 2
    };

    char * str = program_to_str(&p);
    char * expected = "let myVar = anotherVar;\nreturn 5;";

    if (strcmp(str, expected) != 0) {
        abortf("wrong program string. expected \"%s\", got \"%s\"\n", expected, str);
    }

    free(str);
}

void test_identifier_expression() {
    char * input = "foobar;";
    lexer l = {input, 0};
    parser parser = new_parser(&l);
    program p = parse_program(&parser);

    assert_program_size(&p, 1);

    if (p.statements[0].token.type != IDENT) {
        abortf("expected %s, got %s", token_to_str(IDENT), p.statements[0].token.type);
    }

    if (strcmp(p.statements[0].token.literal, "foobar") != 0) {
        abortf("expected %s, got %s", "foobar", p.statements[0].token.literal);
    }

    if (strcmp(p.statements[0].expression.str_value, "foobar") != 0) {
        abortf("expected %s, got %s", "foobar", p.statements[0].expression.str_value);
    }
}


void test_integer_literal(expression expr, int expected) {
    if (expr.int_value != expected) {
        abortf("wrong integer value: expected %d, got %d", expected, expr.int_value);
    }

    // TODO: Test token literal here as well?
}

void test_integer_expression() {
    char * input = "5;";
    lexer l = {input, 0};
    parser parser = new_parser(&l);
    program p = parse_program(&parser);

    assert_parser_errors(&parser);
    assert_program_size(&p, 1);

    statement stmt = p.statements[0];

    if (stmt.token.type != INT) {
        abortf("expected %s, got %s", token_to_str(INT), p.statements[0].token.type);
    }

    if (strcmp(stmt.token.literal, "5") != 0) {
        abortf("expected %s, got %s", "foobar", p.statements[0].token.literal);
    }

    test_integer_literal(stmt.expression, 5);
}

void test_infix_expressions() {
    struct test {
        char input[64];
        int left_value;
        char operator[2];
        int right_value;
    } tests[] = {
       {"5 + 5", 5, "+", 5},
       {"5 - 5", 5, "-", 5},
       {"5 * 5", 5, "*", 5},
       {"5 / 5", 5, "/", 5},
       {"5 > 5", 5, ">", 5},
       {"5 < 5", 5, "<", 5},
       {"5 == 5", 5, "==", 5},
       {"5 != 5", 5, "!=", 5},
    };

    for (int i=0; i < 2; i++) {
        lexer l = {tests[i].input, 0};
        parser parser = new_parser(&l);
        program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        statement stmt = p.statements[0];
       
        test_integer_literal(*stmt.expression.left, tests[i].left_value);
         if (strcmp(stmt.expression.operator, tests[i].operator) != 0) {
            abortf("wrong operator. expected %s, got %s\n", tests[i].operator, stmt.expression.operator);
        }
        test_integer_literal(*stmt.expression.right, tests[i].right_value);
    }
}

void test_prefix_expressions() {
    struct test {
        char input[64];
        char operator[2];
        int int_value;
    } tests[] = {
        {"!5", "!", 5},
        {"-15", "-", 15},
    };

    for (int i=0; i < 2; i++) {
        lexer l = {tests[i].input, 0};
        parser parser = new_parser(&l);
        program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        statement stmt = p.statements[0];
        if (strcmp(stmt.expression.operator, tests[i].operator) != 0) {
            abortf("wrong operator. expected %s, got %s\n", tests[i].operator, stmt.expression.operator);
        }

        test_integer_literal(*stmt.expression.right, tests[i].int_value);
    }
}

int main() {
    test_let_statements();
    test_return_statements();
    test_program_string();
    test_identifier_expression();
    test_integer_expression();
    test_prefix_expressions();
    test_infix_expressions();
}
