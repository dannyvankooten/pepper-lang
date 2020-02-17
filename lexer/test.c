#include "lexer.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#undef EOF

int main() {
    char * input = "let five = 5;\n"
        "let ten = 10;\n"
        "let add = fn(x, y) {\n"
        "x + y;\n"
        "};\n"
        "let result = add(five, ten);\n";

    lexer l;
    l.input = input;
    l.pos = 0;
    token tokens[37];
    strcpy(tokens[0].type, LET);
    strcpy(tokens[0].literal, "let");
    strcpy(tokens[1].type, IDENT);
    strcpy(tokens[1].literal, "five");
    strcpy(tokens[2].type, ASSIGN);
    strcpy(tokens[2].literal, "=");
    strcpy(tokens[3].type, INT);
    strcpy(tokens[3].literal, "5");
    strcpy(tokens[4].type, SEMICOLON);
    strcpy(tokens[4].literal, ";");
    strcpy(tokens[5].type, LET);
    strcpy(tokens[5].literal, "let");
    strcpy(tokens[6].type, IDENT);
    strcpy(tokens[6].literal, "ten");
    strcpy(tokens[7].type, ASSIGN);
    strcpy(tokens[7].literal, "=");
    strcpy(tokens[8].type, INT);
    strcpy(tokens[8].literal, "10");
    strcpy(tokens[9].type, SEMICOLON);
    strcpy(tokens[9].literal, ";");
    strcpy(tokens[10].type, LET);
    strcpy(tokens[10].literal, "let");
    strcpy(tokens[11].type, IDENT);
    strcpy(tokens[11].literal, "add");
    strcpy(tokens[12].type, ASSIGN);
    strcpy(tokens[12].literal, "=");
    strcpy(tokens[13].type, FUNCTION);
    strcpy(tokens[13].literal, "fn");
    strcpy(tokens[14].type, LPAREN);
    strcpy(tokens[14].literal, "(");
    strcpy(tokens[15].type, IDENT);
    strcpy(tokens[15].literal, "x");
    strcpy(tokens[16].type, COMMA);
    strcpy(tokens[16].literal, ",");
    strcpy(tokens[17].type, IDENT);
    strcpy(tokens[17].literal, "y");
    strcpy(tokens[18].type, RPAREN);
    strcpy(tokens[18].literal, ")");
    strcpy(tokens[19].type, LBRACE);
    strcpy(tokens[19].literal, "{");
    strcpy(tokens[20].type, IDENT);
    strcpy(tokens[20].literal, "x");
    strcpy(tokens[21].type, PLUS);
    strcpy(tokens[21].literal, "+");
    strcpy(tokens[22].type, IDENT);
    strcpy(tokens[22].literal, "y");
    strcpy(tokens[23].type, SEMICOLON);
    strcpy(tokens[23].literal, ";");
    strcpy(tokens[24].type, RBRACE);
    strcpy(tokens[24].literal, "}");
    strcpy(tokens[25].type, SEMICOLON);
    strcpy(tokens[25].literal, ";");
    strcpy(tokens[26].type, LET);
    strcpy(tokens[26].literal, "let");
    strcpy(tokens[27].type, IDENT);
    strcpy(tokens[27].literal, "result");
    strcpy(tokens[28].type, ASSIGN);
    strcpy(tokens[28].literal, "=");
    strcpy(tokens[29].type, IDENT);
    strcpy(tokens[29].literal, "add");
    strcpy(tokens[30].type, LPAREN);
    strcpy(tokens[30].literal, "(");
    strcpy(tokens[31].type, IDENT);
    strcpy(tokens[31].literal, "five");
    strcpy(tokens[32].type, COMMA);
    strcpy(tokens[32].literal, ",");
    strcpy(tokens[33].type, IDENT);
    strcpy(tokens[33].literal, "ten");
    strcpy(tokens[34].type, RPAREN);
    strcpy(tokens[34].literal, ")");
    strcpy(tokens[35].type, SEMICOLON);
    strcpy(tokens[35].literal, ";");
    strcpy(tokens[36].type, EOF);
    strcpy(tokens[36].literal, "");

    for (int i = 0; i < 37; i++) {
        token t;
        gettoken(&l, &t);
        
        if (strcmp(t.literal, tokens[i].literal) != 0) {
            printf("[%d] wrong literal: expected \"%s\", got \"%s\"\n", i, tokens[i].literal, t.literal);
            abort();
        }

        if (strcmp(t.type, tokens[i].type) != 0) {
            printf("[%d] wrong type: expected \"%s\", got \"%s\"\n", i, tokens[i].type, t.type);
            abort();
        }

    }


}
