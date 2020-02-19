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

   exit(1);
}

void assert_parser_errors(parser *p) {
    if (p->errors > 0) {
        printf("parser has %d errors: \n", p->errors);
        for (int i = 0; i < p->errors; i++) {
            printf("  - %s\n", p->error_messages[i]);
        }

        exit(1);
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
    expression e1 = {
        .token = {
            .type = IDENT,
            .literal = "anotherVar",
        },
        .str_value = "anotherVar",
    };
    expression e2 = {
        .token = {
            .type = INT,
            .literal = "5",
        },
        .int_value = 5,
    };
    statement statements[2] = {
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
            .value = &e1,
        }, 
        {
            .token = {
                .type = RETURN,
                .literal = "return",
            },
            .value = &e2
        }, 
    };
    program p = {
        .statements = statements, 
        .size = 2
    };

    char * str = program_to_str(&p);
    char * expected = "let myVar = anotherVar;return 5;";

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

    statement stmt = p.statements[0];

    if (stmt.token.type != IDENT) {
        abortf("wrong token type: expected %s, got %s\n", token_to_str(IDENT), stmt.token.type);
    }

    if (strcmp(stmt.token.literal, "foobar") != 0) {
        abortf("wrong token literal: expected %s, got %s\n", "foobar", stmt.token.literal);
    }

    if (stmt.value->type != EXPR_IDENT) {
        abortf("wrong expression type: expected %d, got %d\n", EXPR_IDENT, stmt.value->type);
    }

    if (strcmp(stmt.value->str_value, "foobar") != 0) {
        abortf("wrong expression value: expected \"%s\", got \"%s\"\n", "foobar", stmt.value->str_value);
    }
}


void test_integer_literal(expression * expr, int expected) {

    if (expr->type != EXPR_INT) {
        abortf("wrong expression type: expected %d, got %d\n", EXPR_INT, expr->type);
    }

    if (expr->int_value != expected) {
        abortf("wrong integer value: expected %d, got %d\n", expected, expr->int_value);
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
        abortf("wroken token type: expected %s, got %s", token_to_str(INT), p.statements[0].token.type);
    }

    if (strcmp(stmt.token.literal, "5") != 0) {
        abortf("wrong token literal: expected %s, got %s", "foobar", p.statements[0].token.literal);
    }

    test_integer_literal(stmt.value, 5);
}

void test_infix_expressions() {
    struct test {
        char input[16];
        int left_value;
        char operator[4];
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

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        lexer l = {tests[i].input, 0};
        parser parser = new_parser(&l);
        program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        statement stmt = p.statements[0];
       
        test_integer_literal(stmt.value->left, tests[i].left_value);
        if (strncmp(stmt.value->operator, tests[i].operator, 2) != 0) {
            abortf("wrong operator: expected %s, got %s\n", tests[i].operator, stmt.value->operator);
        }
        test_integer_literal(stmt.value->right, tests[i].right_value);
    }
}

void test_prefix_expressions() {
    struct test {
        char input[64];
        char operator[4];
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

        if (strncmp(stmt.value->operator, tests[i].operator, 2) != 0) {
            abortf("wrong operator. expected %s, got %s\n", tests[i].operator, stmt.value->operator);
        }

        test_integer_literal(stmt.value->right, tests[i].int_value);
    }
}

void test_operator_precedence_parsing() {
    struct test {
        char input[32];
        char expected[48];
    } tests[] = {
       {"-a * b", "((-a) * b)"},
       {"!-a", "(!(-a))"},
       {"a + b + c", "((a + b) + c)"},
       {"a + b - c", "((a + b) - c)"},
       {"a * b * c", "((a * b) * c)"},
       {"a * b / c", "((a * b) / c)"},
       {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
       {"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
       {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
       {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
       {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        lexer l = {tests[i].input, 0};
        parser parser = new_parser(&l);
        program p = parse_program(&parser);

        assert_parser_errors(&parser);
        
        char *program_str = program_to_str(&p);
        if (strncmp(program_str, tests[i].expected, 48) != 0) {
            abortf("incorrect program string: expected %s, got %s\n", tests[i].expected, program_str);
        }
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
    test_operator_precedence_parsing();
    printf("\x1b[32mAll tests passed!\n");
}
