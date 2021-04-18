#include "../src/lexer.h"
#include "test_helpers.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void test_lexer() {
    const char *input = "let five = 5;\n"
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
        "10 != 9;\n"
        "\"foobar\"\n"
        "\"foo bar\""
        "[1, 2];"
        "[\"one\", \"two\"];\n"
        "varname; // comment\n"
        "// varname\n"
        "\"string\\\" with escaped quote\""
    ;

    struct lexer l = {input, 0};
    struct token tokens[] = {
        {TOKEN_LET, "let"},
        {TOKEN_IDENT, "five"},
        {TOKEN_ASSIGN, "="},
        {TOKEN_INT, "5"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_LET, "let"},
        {TOKEN_IDENT, "ten"},
        {TOKEN_ASSIGN, "="},
        {TOKEN_INT, "10"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_LET, "let"},
        {TOKEN_IDENT, "add"},
        {TOKEN_ASSIGN, "="},
        {TOKEN_FUNCTION, "fn"},
        {TOKEN_LPAREN, "("},
        {TOKEN_IDENT, "x"},
        {TOKEN_COMMA, ","},
        {TOKEN_IDENT, "y"},
        {TOKEN_RPAREN, ")"},
        {TOKEN_LBRACE, "{"},
        {TOKEN_IDENT, "x"},
        {TOKEN_PLUS, "+"},
        {TOKEN_IDENT, "y"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_RBRACE, "}"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_LET, "let"},
        {TOKEN_IDENT, "result"},
        {TOKEN_ASSIGN, "="},
        {TOKEN_IDENT, "add"},
        {TOKEN_LPAREN, "("},
        {TOKEN_IDENT, "five"},
        {TOKEN_COMMA, ","},
        {TOKEN_IDENT, "ten"},
        {TOKEN_RPAREN, ")"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_BANG, "!"},
        {TOKEN_MINUS, "-"},
        {TOKEN_SLASH, "/"},
        {TOKEN_ASTERISK, "*"},
        {TOKEN_INT, "5"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_INT, "5"},
        {TOKEN_LT, "<"},
        {TOKEN_INT, "10"},
        {TOKEN_GT, ">"},
        {TOKEN_INT, "5"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_IF, "if"},
        {TOKEN_LPAREN, "("},
        {TOKEN_INT, "5"},
        {TOKEN_LT, "<"},
        {TOKEN_INT, "10"},
        {TOKEN_RPAREN, ")"},
        {TOKEN_LBRACE, "{"},
        {TOKEN_RETURN, "return"},
        {TOKEN_TRUE, "true"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_RBRACE, "}"},
        {TOKEN_ELSE, "else"},
        {TOKEN_LBRACE, "{"},
        {TOKEN_RETURN, "return"},
        {TOKEN_FALSE, "false"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_RBRACE, "}"},
        {TOKEN_INT, "10"},
        {TOKEN_EQ, "=="},
        {TOKEN_INT, "10"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_INT, "10"},
        {TOKEN_NOT_EQ, "!="},
        {TOKEN_INT, "9"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_STRING, "", "foobar"},
        {TOKEN_STRING, "", "foo bar"},
        {TOKEN_LBRACKET, "["},
        {TOKEN_INT, "1"},
        {TOKEN_COMMA, ","},
        {TOKEN_INT, "2"},
        {TOKEN_RBRACKET, "]"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_LBRACKET, "["},
        {TOKEN_STRING, "", "one"},
        {TOKEN_COMMA, ","},
        {TOKEN_STRING, "", "two"},
        {TOKEN_RBRACKET, "]"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_IDENT, "varname"},
        {TOKEN_SEMICOLON, ";"},
        {TOKEN_STRING, "", "string\" with escaped quote"},
        {TOKEN_EOF, ""},
    };

    struct token t;

    for (int j = 0; j < sizeof tokens / sizeof tokens[0]; j++) {
        gettoken(&l, &t);
        assertf(t.type == tokens[j].type, "[%d] wrong type: expected \"%s\", got \"%s\"\n", j, token_type_to_str(tokens[j].type), token_type_to_str(t.type));
        assertf(strcmp(t.literal, tokens[j].literal) == 0, "[%d] wrong %s literal: expected \"%s\", got \"%s\"\n", j, token_type_to_str(tokens[j].type), tokens[j].literal, t.literal);
    }
}

void string_with_256_chars() {
    char *input = "\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"";
    struct lexer l = new_lexer(input);
    struct token t;
    gettoken(&l, &t);
    assertf(t.type == TOKEN_STRING, "wrong type");
    assertf(strncmp(t.start, &input[1], t.end - t.start) == 0, "wrong value");
}

int main(int argc, char *argv[]) {
    TEST(test_lexer);
    TEST(string_with_256_chars);
}
