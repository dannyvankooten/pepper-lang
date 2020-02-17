#include "../token/token.h"
#include <string.h>
#include <stdio.h>
#undef EOF

typedef struct Lexer {
    char * input;
    unsigned int pos;
} lexer;

int is_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') 
        || (ch >= 'A' && ch <= 'Z')
        || (ch == '_');
}

int is_digit(char ch) {
    return (ch >= '0' && ch <= '9');
}

int gettoken(lexer *l, token *t) {
    char ch = l->input[l->pos++];
    
    // skip whitespace
    while (ch == ' ' || ch == '\n' || ch == '\t') {
        ch = l->input[l->pos++];
    }

    char ch_next = l->input[l->pos];

    switch (ch) {
        case '=':
            if (ch_next == '=') {
                strcpy(t->type, EQ);
                strcpy(t->literal, "==");
                l->pos++;
            } else {
                strcpy(t->type, ASSIGN);
                t->literal[0] = ch;
                t->literal[1] = '\0';
            }
        break;

        case ';':
            strcpy(t->type, SEMICOLON);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '(':
            strcpy(t->type, LPAREN);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;
        
        case ')':
            strcpy(t->type, RPAREN);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case ',':
            strcpy(t->type, COMMA);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '+':
            strcpy(t->type, PLUS);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '-':
            strcpy(t->type, MINUS);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '!':
            if (ch_next == '=') {
                strcpy(t->type, NOT_EQ);
                strcpy(t->literal, "!=");
                l->pos++;
            } else {
                strcpy(t->type, BANG);
                t->literal[0] = ch;
                t->literal[1] = '\0';
            }
        break;

        case '/':
            strcpy(t->type, SLASH);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '*':
            strcpy(t->type, ASTERISK);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '<':
            strcpy(t->type, LT);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '>':
            strcpy(t->type, GT);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '{':
            strcpy(t->type, LBRACE);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;

        case '}':
            strcpy(t->type, RBRACE);
            t->literal[0] = ch;
            t->literal[1] = '\0';
        break;



        default:
            if (is_letter(ch)) {    
                int i = 0; 
                while (is_letter(ch)) {
                    t->literal[i++] = ch;
                    ch = l->input[l->pos++];
                }
                t->literal[i++] = '\0';
                get_ident(t);

                 // return last character to input 
                l->pos--;
            } else if (is_digit(ch)) {
                int i = 0;
                while (is_digit(ch)) {
                    t->literal[i++] = ch;
                    ch = l->input[l->pos++];
                }
                t->literal[i++] = '\0';
                strcpy(t->type, INT);

                // return last character to input 
                l->pos--;
            } else {
                strcpy(t->type, ILLEGAL);
                t->literal[0] = ch;  
            }

            
            
            break;

            case '\0':
                strcpy(t->type, EOF);
                t->literal[0] = '\0';
                return -1; // signal DONE
            break;
    }

    return 1;
}
