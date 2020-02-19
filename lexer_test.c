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

    struct lexer l;
    l.input = input;
    l.pos = 0;
    struct token tokens[100];
    tokens[0].type = LET;
    strcpy(tokens[0].literal, "let");
    tokens[1].type = IDENT;
    strcpy(tokens[1].literal, "five");
    tokens[2].type = ASSIGN;
    strcpy(tokens[2].literal, "=");
    tokens[3].type = INT;
    strcpy(tokens[3].literal, "5");
    tokens[4].type = SEMICOLON;
    strcpy(tokens[4].literal, ";");
    tokens[5].type = LET;
    strcpy(tokens[5].literal, "let");
    tokens[6].type = IDENT;
    strcpy(tokens[6].literal, "ten");
    tokens[7].type = ASSIGN;
    strcpy(tokens[7].literal, "=");
    tokens[8].type = INT;
    strcpy(tokens[8].literal, "10");
    tokens[9].type = SEMICOLON;
    strcpy(tokens[9].literal, ";");
    tokens[10].type = LET;
    strcpy(tokens[10].literal, "let");
    tokens[11].type = IDENT;
    strcpy(tokens[11].literal, "add");
    tokens[12].type = ASSIGN;
    strcpy(tokens[12].literal, "=");
    tokens[13].type = FUNCTION;
    strcpy(tokens[13].literal, "fn");
    tokens[14].type = LPAREN;
    strcpy(tokens[14].literal, "(");
    tokens[15].type = IDENT;
    strcpy(tokens[15].literal, "x");
    tokens[16].type = COMMA;
    strcpy(tokens[16].literal, ",");
    tokens[17].type = IDENT;
    strcpy(tokens[17].literal, "y");
    tokens[18].type = RPAREN;
    strcpy(tokens[18].literal, ")");
    tokens[19].type = LBRACE;
    strcpy(tokens[19].literal, "{");
    tokens[20].type = IDENT;
    strcpy(tokens[20].literal, "x");
    tokens[21].type = PLUS;
    strcpy(tokens[21].literal, "+");
    tokens[22].type = IDENT;
    strcpy(tokens[22].literal, "y");
    tokens[23].type = SEMICOLON;
    strcpy(tokens[23].literal, ";");
    tokens[24].type = RBRACE;
    strcpy(tokens[24].literal, "}");
    tokens[25].type = SEMICOLON;
    strcpy(tokens[25].literal, ";");
    tokens[26].type = LET;
    strcpy(tokens[26].literal, "let");
    tokens[27].type = IDENT;
    strcpy(tokens[27].literal, "result");
    tokens[28].type = ASSIGN;
    strcpy(tokens[28].literal, "=");
    tokens[29].type = IDENT;
    strcpy(tokens[29].literal, "add");
    tokens[30].type = LPAREN;
    strcpy(tokens[30].literal, "(");
    tokens[31].type = IDENT;
    strcpy(tokens[31].literal, "five");
    tokens[32].type = COMMA;
    strcpy(tokens[32].literal, ",");
    tokens[33].type = IDENT;
    strcpy(tokens[33].literal, "ten");
    tokens[34].type = RPAREN;
    strcpy(tokens[34].literal, ")");
    tokens[35].type = SEMICOLON;
    strcpy(tokens[35].literal, ";");
    tokens[36].type = BANG;
    strcpy(tokens[36].literal, "!");
    tokens[37].type = MINUS;
    strcpy(tokens[37].literal, "-");
    tokens[38].type = SLASH;
    strcpy(tokens[38].literal, "/");
    tokens[39].type = ASTERISK;
    strcpy(tokens[39].literal, "*");
    tokens[40].type = INT;
    strcpy(tokens[40].literal, "5");
    tokens[41].type = SEMICOLON;
    strcpy(tokens[41].literal, ";");
    tokens[42].type = INT;
    strcpy(tokens[42].literal, "5");
    tokens[43].type = LT;
    strcpy(tokens[43].literal, "<");
    tokens[44].type = INT;
    strcpy(tokens[44].literal, "10");
    tokens[45].type = GT;
    strcpy(tokens[45].literal, ">");
    tokens[46].type = INT;
    strcpy(tokens[46].literal, "5");
    tokens[47].type = SEMICOLON;
    strcpy(tokens[47].literal, ";");
    tokens[48].type = IF;
    strcpy(tokens[48].literal, "if");
    tokens[49].type = LPAREN;
    strcpy(tokens[49].literal, "(");
    int i = 50;
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "5");
    tokens[i].type = LT;
    strcpy(tokens[i++].literal, "<");
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "10");
    tokens[i].type = RPAREN;
    strcpy(tokens[i++].literal, ")");
    tokens[i].type = LBRACE;
    strcpy(tokens[i++].literal, "{");
    tokens[i].type = RETURN;
    strcpy(tokens[i++].literal, "return");
    tokens[i].type = TRUE;
    strcpy(tokens[i++].literal, "true");
    tokens[i].type = SEMICOLON;
    strcpy(tokens[i++].literal, ";");
    tokens[i].type = RBRACE;
    strcpy(tokens[i++].literal, "}");
    tokens[i].type = ELSE;
    strcpy(tokens[i++].literal, "else");
    tokens[i].type = LBRACE;
    strcpy(tokens[i++].literal, "{");
    tokens[i].type = RETURN;
    strcpy(tokens[i++].literal, "return");
    tokens[i].type = FALSE;
    strcpy(tokens[i++].literal, "false");
    tokens[i].type = SEMICOLON;
    strcpy(tokens[i++].literal, ";");
    tokens[i].type = RBRACE;
    strcpy(tokens[i++].literal, "}");
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "10");
    tokens[i].type = EQ;
    strcpy(tokens[i++].literal, "==");
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "10");
    tokens[i].type = SEMICOLON;
    strcpy(tokens[i++].literal, ";");
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "10");
    tokens[i].type = NOT_EQ;
    strcpy(tokens[i++].literal, "!=");
    tokens[i].type = INT;
    strcpy(tokens[i++].literal, "9");
    tokens[i].type = SEMICOLON;
    strcpy(tokens[i++].literal, ";");
    tokens[i].type = EOF;
    strcpy(tokens[i++].literal, "");

    for (int j = 0; j < i; j++) {
       struct token t;
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
