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

typedef union {
    int int_value;
    char bool_value;
    char * str_value;
} expression_value;

void assert_parser_errors(struct parser *p) {
    if (p->errors > 0) {
        printf("parser has %d errors: \n", p->errors);
        for (int i = 0; i < p->errors; i++) {
            printf("  - %s\n", p->error_messages[i]);
        }

        exit(1);
    }
}

void assert_program_size(struct program *p, unsigned expected_size) {
    if (p->size != expected_size) {
        abortf("wrong program size. expected %d, got %d\n", expected_size, p->size);
    }   
}

void test_let_statements() {
    char * input = ""
        "let x = 5;\n"
        "let y = 10;\n"
        "let foo = 838383;\n";

    struct lexer l = {input, 0 };
    struct parser parser = new_parser(&l);
    struct program p = parse_program(&parser);
    
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

    struct lexer l = {input, 0 };
    struct parser parser = new_parser(&l);
    struct program p = parse_program(&parser);
    
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
    struct expression e1 = {
        .type = EXPR_IDENT,
        .ident = {
            .token = {
                .type = IDENT,
                .literal = "anotherVar",
            },
            .value = "anotherVar"
        }
    };
    struct expression e2 = {
        .type = EXPR_INT,
        ._int = {
            .token = {
                .type = INT,
                .literal = "5",
            },
            .value = 5
        }
    };
    struct statement statements[] = {
        {
            .type = STMT_LET,
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
            .type = STMT_RETURN,
            .token = {
                .type = RETURN,
                .literal = "return",
            },
            .value = &e2
        }, 
    };
    struct program p = {
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

void test_identifier_expression(struct expression *e, char * expected) {
    if (e->type != EXPR_IDENT) {
        abortf("wrong expression type: expected %d, got %d\n", EXPR_IDENT, e->type);
    }

    if (strcmp(e->ident.token.literal, expected) != 0) {
        abortf("wrong token literal: expected \"%s\", got \"%s\"\n", expected, e->ident.token.literal);
    }

    if (strcmp(e->ident.value, expected) != 0) {
        abortf("wrong expression value: expected \"%s\", got \"%s\"\n", expected, e->ident.value);
    }
}


void test_identifier_expression_parsing() {
    char * input = "foobar;";
    struct lexer l = {input, 0};
    struct parser parser = new_parser(&l);
    struct program p = parse_program(&parser);

    assert_program_size(&p, 1);

    struct statement stmt = p.statements[0];

    if (stmt.token.type != IDENT) {
        abortf("wrong token type: expected %s, got %s\n", token_to_str(IDENT), stmt.token.type);
    }

    if (strcmp(stmt.token.literal, "foobar") != 0) {
        abortf("wrong token literal: expected %s, got %s\n", "foobar", stmt.token.literal);
    }

    test_identifier_expression(stmt.value, "foobar");
}


void test_integer_expression(struct expression * expr, int expected) {
    if (expr->type != EXPR_INT) {
        abortf("wrong expression type: expected %d, got %d\n", EXPR_INT, expr->type);
    }

    if (expr->_int.value != expected) {
        abortf("wrong integer value: expected %d, got %d\n", expected, expr->_int.value);
    }

    char expected_str[8];
    sprintf(expected_str, "%d", expected);
    if (strcmp(expr->_int.token.literal, expected_str) != 0) {
        abortf("wrong token literal: expected %s, got %s\n", expected_str, expr->_int.token.literal);
    }
}


void test_integer_expression_parsing() {
    char * input = "5;";
    struct lexer l = {input, 0};
    struct parser parser = new_parser(&l);
    struct program p = parse_program(&parser);

    assert_parser_errors(&parser);
    assert_program_size(&p, 1);

    struct statement stmt = p.statements[0];

    if (stmt.token.type != INT) {
        abortf("wroken token type: expected %s, got %s", token_to_str(INT), p.statements[0].token.type);
    }

    if (strcmp(stmt.token.literal, "5") != 0) {
        abortf("wrong token literal: expected %s, got %s", "foobar", p.statements[0].token.literal);
    }

    test_integer_expression(stmt.value, 5);
}


void test_boolean_expression(struct expression * expr, char expected) {
    if (expr->type != EXPR_BOOL) {
        abortf("wrong expression type: expected %d, got %d\n", EXPR_BOOL, expr->type);
    }

    if (expr->bool.value != expected) {
        abortf("wrong boolean value: expected %d, got %d\n", expected, expr->bool.value);
    }

    char * expected_str = expected ? "true" : "false";
    if (strcmp(expr->bool.token.literal, expected_str) != 0) {
        abortf("wrong token literal: expected %s, got %s\n", expected_str, expr->bool.token.literal);
    }
}

void test_boolean_expression_parsing() {
    struct test {
        char * input;
        char expected;
    } tests[] = {
      {"true;", 1},
      {"false;", 0}
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        struct statement stmt = p.statements[0];

        test_boolean_expression(stmt.value, tests[i].expected);
    }
}

void test_expression(struct expression *e, expression_value expected) {
    switch (e->type) {
        case EXPR_BOOL: return test_boolean_expression(e, expected.bool_value); break;
        case EXPR_INT: return test_integer_expression(e, expected.int_value); break;
        default: break;
    }
}

void test_infix_expression(struct expression *expr, expression_value left_value, char *operator, expression_value right_value) {
    if (expr->type != EXPR_INFIX) {
        abortf("wrong expression type. expected %d, got %d\n", EXPR_INFIX, expr->type);
    }

    test_expression(expr->infix.left, left_value);
    if (strncmp(expr->infix.operator, operator, MAX_OPERATOR_LENGTH) != 0) {
        abortf("wrong operator: expected %s, got %s\n", operator, expr->infix.operator);
    }
    test_expression(expr->infix.right, right_value);
}

void test_infix_expression_parsing() {
    typedef struct {
        char *input;
        expression_value left_value;
        char *operator;
        expression_value right_value;        
    } test;
    test tests[] = {
       {"5 + 5", {5}, "+", {5}},
       {"5 - 5", {5}, "-", {5}},
       {"5 * 5", {5}, "*", {5}},
       {"5 / 5", {5}, "/", {5}},
       {"5 > 5", {5}, ">", {5}},
       {"5 < 5", {5}, "<", {5}},
       {"5 == 5", {5}, "==", {5}},
       {"5 != 5", {5}, "!=", {5}},
       {"true == true", {1}, "==", {1}},
       {"true != false", {1}, "!=", {0}},
       {"false == false", {0}, "==", {0}},
    };
    test t;

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        t = tests[i];
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        struct statement stmt = p.statements[0];

        test_infix_expression(stmt.value, t.left_value, t.operator, t.right_value);
    }
}

void test_prefix_expression_parsing() {
    typedef struct test {
        char *input;
        char *operator;
        expression_value value;
    } test;

    test tests[] = {
        {"!5", "!", { .int_value = 5 }},
        {"-15", "-", { .int_value = 15 }},
        {"!true", "!", { .bool_value = 1 }},
        {"!false", "!", { .bool_value = 0 }},
    };
    test t;
    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        t = tests[i];
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program p = parse_program(&parser);

        assert_parser_errors(&parser);
        assert_program_size(&p, 1);
        struct statement stmt = p.statements[0];

        if (stmt.value->type != EXPR_PREFIX) {
            abortf("wrong expression type. expected %d, got %d\n", EXPR_PREFIX, stmt.value->type);
        }

        if (strncmp(stmt.value->prefix.operator, t.operator, 2) != 0) {
            abortf("wrong operator. expected %s, got %s\n", t.operator, stmt.value->prefix.operator);
        }   

        test_expression(stmt.value->prefix.right, t.value);        
    }
}

void test_operator_precedence_parsing() {
    struct test {
        char *input;
        char *expected;
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
       {"true", "true"},
       {"false", "false"},
       {"3 > 5 == false", "((3 > 5) == false)"},
       {"3 < 5 == true", "((3 < 5) == true)"},
       {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
       {"(5 + 5) * 2", "((5 + 5) * 2)"},
       {"2 / ( 5 + 5)", "(2 / (5 + 5))"},
       {"-(5 + 5)", "(-(5 + 5))"},
       {"!(true == true)", "(!(true == true))"},
    };

    for (int i=0; i < sizeof tests / sizeof tests[0]; i++) {
        struct lexer l = {tests[i].input, 0};
        struct parser parser = new_parser(&l);
        struct program p = parse_program(&parser);

        assert_parser_errors(&parser);
        
        char *program_str = program_to_str(&p);
        if (strncmp(program_str, tests[i].expected, 48) != 0) {
            abortf("wrong program string: expected %s, got %s\n", tests[i].expected, program_str);
        }
    }
}

void test_if_expression_parsing() {
    char *input = "if (x < y) { x }";
    struct lexer lexer = {input};
    struct parser parser = new_parser(&lexer);
    struct program program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(&program, 1);

    struct statement stmt = program.statements[0];
    struct expression *expr = stmt.value;
    if (expr->type != EXPR_IF) {
        abortf("invalid statement type: expected %d, got %d\n", EXPR_IF, stmt.type);
    }

    expression_value left = {.str_value = "x"};
    expression_value right = {.str_value = "y"};
    test_infix_expression(expr->_if.condition, left, "<", right);

    struct block_statement *consequence = expr->_if.consequence;
    if (!consequence) {
        abortf("expected consequence block statement, got NULL\n");
    }

    if (consequence->size != 1) {
        abortf("invalid consequence size: expected %d, got %d\n", 1, consequence->size);
    }

    if (consequence->statements[0].type != STMT_EXPR) {
        abortf("statements[0] is not a statement expression, got %d\n", consequence->statements[0].type);
    }

    test_identifier_expression(consequence->statements[0].value, "x");

    if (expr->_if.alternative) {
        abortf("expected NULL, got alternative block statement\n");
    }
}

void test_if_else_expression_parsing() {
    char *input = "if (x < y) { x } else { 5 }";
    struct lexer lexer = {input};
    struct parser parser = new_parser(&lexer);
    struct program program = parse_program(&parser);
    assert_parser_errors(&parser);
    assert_program_size(&program, 1);

    struct statement stmt = program.statements[0];
    struct expression *expr = stmt.value;
    if (expr->type != EXPR_IF) {
        abortf("invalid statement type: expected %d, got %d\n", EXPR_IF, stmt.type);
    }

    expression_value left = {.str_value = "x"};
    expression_value right = {.str_value = "y"};
    test_infix_expression(expr->_if.condition, left, "<", right);

    struct block_statement *consequence = expr->_if.consequence;
    if (!consequence) {
        abortf("expected consequence block statement, got NULL\n");
    }

    if (consequence->size != 1) {
        abortf("invalid consequence size: expected %d, got %d\n", 1, consequence->size);
    }

    if (consequence->statements[0].type != STMT_EXPR) {
        abortf("statements[0] is not a statement expression, got %d", consequence->statements[0].type);
    }

    test_identifier_expression(consequence->statements[0].value, "x");

    struct block_statement *alternative = expr->_if.alternative;
    if (alternative == NULL) {
        abortf("alternative was NULL");
    }

    if (alternative->size != 1) {
        abortf("invalid alternative size: expected %d, got %d\n", 1, alternative->size);
    }
}

int main() {
    test_let_statements();
    test_return_statements();
    test_program_string();
    test_identifier_expression_parsing();
    test_integer_expression_parsing();
    test_boolean_expression_parsing();
    test_prefix_expression_parsing();
    test_infix_expression_parsing();
    test_operator_precedence_parsing();
    test_if_expression_parsing();
    test_if_else_expression_parsing();
    printf("\x1b[32mAll tests passed!\n");
}
