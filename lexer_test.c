#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#undef EOF

int main() {
    char * input = "let five = 5;\n"
        "let ten = 10;\n"
        "let add = fn(x, y) {\n"
        "\tx + y;\n"
        "};\n"
        "let result = add(five, ten);\n"
        "!-/*5;\n"
        "5 < 10 > 5;\n"
        "if (5 < 10) {\n"
        "\treturn true;\n"
        "} else {\n"
        "\treturn false;\n"
        "}\n"
        "10 == 10;\n"
        "10 != 9;\n";

    struct lexer l = {input, 0};
    struct token tokens[] = {
        {LET, "let"},
        {IDENT, "five"},
        {ASSIGN, "="},
        {INT, "5"},
        {SEMICOLON, ";"},
        {LET, "let"},
        {IDENT, "ten"},
        {ASSIGN, "="},
        {INT, "10"},
        {SEMICOLON, ";"},
        {LET, "let"},
        {IDENT, "add"},
        {ASSIGN, "="},
        {FUNCTION, "fn"},
        {LPAREN, "("},
        {IDENT, "x"},
        {COMMA, ","},
        {IDENT, "y"},
        {RPAREN, ")"},
        {LBRACE, "{"},
        {IDENT, "x"},
        {PLUS, "+"},
        {IDENT, "y"},
        {SEMICOLON, ";"},
        {RBRACE, "}"},
        {SEMICOLON, ";"},
        {LET, "let"},
        {IDENT, "result"},
        {ASSIGN, "="},
        {IDENT, "add"},
        {LPAREN, "("},
        {IDENT, "five"},
        {COMMA, ","},
        {IDENT, "ten"},
        {RPAREN, ")"},
        {SEMICOLON, ";"},
        {BANG, "!"},
        {MINUS, "-"},
        {SLASH, "/"},
        {ASTERISK, "*"},
        {INT, "5"},
        {SEMICOLON, ";"},
        {INT, "5"},
        {LT, "<"},
        {INT, "10"},
        {GT, ">"},
        {INT, "5"},
        {SEMICOLON, ";"},
        {IF, "if"},
        {LPAREN, "("},
        {INT, "5"},
        {LT, "<"},
        {INT, "10"},
        {RPAREN, ")"},
        {LBRACE, "{"},
        {RETURN, "return"},
        {TRUE, "true"},
        {SEMICOLON, ";"},
        {RBRACE, "}"},
        {ELSE, "else"},
        {LBRACE, "{"},
        {RETURN, "return"},
        {FALSE, "false"},
        {SEMICOLON, ";"},
        {RBRACE, "}"},
        {INT, "10"},
        {EQ, "=="},
        {INT, "10"},
        {SEMICOLON, ";"},
        {INT, "10"},
        {NOT_EQ, "!="},
        {INT, "9"},
        {SEMICOLON, ";"},
        {EOF, ""},
    };

    struct token t;
    for (int j = 0; j < sizeof tokens / sizeof tokens[0]; j++) {
        gettoken(&l, &t);

        if (t.type != tokens[j].type) {
            printf("[%d] wrong type: expected \"%d\", got \"%d\"\n", j, tokens[j].type, t.type);
            abort();
        }
        
        if (strcmp(t.literal, tokens[j].literal) != 0) {
            printf("[%d] wrong literal: expected \"%s\", got \"%s\"\n", j, tokens[j].literal, t.literal);
            abort();
        }
    }
}
