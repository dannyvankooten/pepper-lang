#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void get_ident(struct token *t) {
  if (strcmp(t->literal, "let") == 0) {
    t->type = TOKEN_LET;
  } else if (strcmp(t->literal, "fn") == 0) {
    t->type = TOKEN_FUNCTION;
  } else if (strcmp(t->literal, "true") == 0) {
    t->type = TOKEN_TRUE;
  } else if (strcmp(t->literal, "false") == 0) {
    t->type = TOKEN_FALSE;
  } else if (strcmp(t->literal, "if") == 0) {
    t->type = TOKEN_IF;
  } else if (strcmp(t->literal, "else") == 0) {
    t->type = TOKEN_ELSE;
  } else if (strcmp(t->literal, "return") == 0) {
    t->type = TOKEN_RETURN;
  } else if (strcmp(t->literal, "while") == 0) {
    t->type = TOKEN_WHILE;
  } else if (strcmp(t->literal, "for") == 0) {
    t->type = TOKEN_FOR;
  } else if (strcmp(t->literal, "in") == 0) {
    t->type = TOKEN_IN;
  } else if (strcmp(t->literal, "break") == 0) {
    t->type = TOKEN_BREAK;
  } else if (strcmp(t->literal, "continue") == 0) {
    t->type = TOKEN_CONTINUE;
  } else {
    // not a keyword, so assume identifier
    t->type = TOKEN_IDENT;
  }
}

const char *token_type_to_str(const enum token_type type) {
  static const char *token_names[] = {
      "ILLEGAL", "EOF", "IDENT", "INT",   "FUNCTION", "LET",    "TRUE",
      "FALSE",   "IF",  "ELSE",  "FOR",   "IN",       "WHILE",  "RETURN",
      "=",       "+",   "-",     "!",     "*",        "/",      "%",
      "<",       "<=",  ">",     ">=",    "==",       "!=",     ",",
      ";",       "(",   ")",     "{",     "}",        "STRING", "[",
      "]",       "&&",  "||",    "BREAK", "CONTINUE",
  };
  return token_names[type];
}

static bool is_letter(const char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

int gettoken(struct lexer *l, struct token *t) {
  char ch = l->input[l->pos++];

  t->literal[0] = '\0';

  // skip whitespace
  while (isspace(ch)) {
    if (ch == '\n') {
      l->cur_lineno++;
    }

    ch = l->input[l->pos++];
  }

  char ch_next;

  switch (ch) {
  case '=':
    ch_next = l->input[l->pos];
    if (ch_next == '=') {
      t->type = TOKEN_EQ;
      strcpy(t->literal, "==");
      l->pos++;
    } else {
      t->type = TOKEN_ASSIGN;
      t->literal[0] = ch;
      t->literal[1] = '\0';
    }
    break;

  case ';':
    t->type = TOKEN_SEMICOLON;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '(':
    t->type = TOKEN_LPAREN;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case ')':
    t->type = TOKEN_RPAREN;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case ',':
    t->type = TOKEN_COMMA;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '+':
    t->type = TOKEN_PLUS;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '-':
    t->type = TOKEN_MINUS;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '!':
    ch_next = l->input[l->pos];
    if (ch_next == '=') {
      t->type = TOKEN_NOT_EQ;
      strcpy(t->literal, "!=");
      l->pos++;
    } else {
      t->type = TOKEN_BANG;
      t->literal[0] = ch;
      t->literal[1] = '\0';
    }
    break;

  case '/':
    ch_next = l->input[l->pos];

    // 2 consecutive forward slashes starts a comment until EOL
    // skip characters, cause no parsing is needed for comments
    if (ch_next == '/') {
      while (l->input[l->pos] != '\n' && l->input[l->pos] != '\0') {
        l->pos++;
      }
      return gettoken(l, t);
    }
    t->type = TOKEN_SLASH;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '*':
    t->type = TOKEN_ASTERISK;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '<':
    ch_next = l->input[l->pos];
    if (ch_next == '=') {
      l->pos++;
      t->type = TOKEN_LTE;
      strcpy(t->literal, "<=");
    } else {
      t->type = TOKEN_LT;
      strcpy(t->literal, "<");
    }
    break;

  case '>':
    ch_next = l->input[l->pos];
    if (ch_next == '=') {
      l->pos++;
      t->type = TOKEN_GTE;
      strcpy(t->literal, ">=");
    } else {
      t->type = TOKEN_GT;
      strcpy(t->literal, ">");
    }
    break;

  case '{':
    t->type = TOKEN_LBRACE;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '}':
    t->type = TOKEN_RBRACE;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '[':
    t->type = TOKEN_LBRACKET;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case ']':
    t->type = TOKEN_RBRACKET;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '%':
    t->type = TOKEN_PERCENT;
    t->literal[0] = ch;
    t->literal[1] = '\0';
    break;

  case '|':
    ch_next = l->input[l->pos];
    if (ch_next == '|') {
      t->type = TOKEN_OR;
      strcpy(t->literal, "||");
      l->pos++;
    }
  break;

  case '&':
    ch_next = l->input[l->pos];
    if (ch_next == '&') {
      t->type = TOKEN_AND;
      strcpy(t->literal, "&&");
      l->pos++;
    }
  break;

  case '"': {
    t->type = TOKEN_STRING;
    t->start = &l->input[l->pos];
    while (1) {
      ch = l->input[l->pos++];
      if (ch == '\0' || (ch == '"' && l->input[l->pos - 2] != '\\')) {
        break;
      }
    }
    t->end = &l->input[l->pos - 1];
  } break;

  default: {
    if (is_letter(ch)) {
      int32_t i = 0;
      while ((is_letter(ch) || isdigit(ch)) && i < MAX_IDENT_LENGTH - 1) {
        t->literal[i++] = ch;
        ch = l->input[l->pos++];
      }
      t->literal[i] = '\0';
      get_ident(t);

      // return last character to input
      l->pos--;
    } else if (isdigit(ch)) {
      int32_t i = 0;
      while (isdigit(ch) && i < MAX_IDENT_LENGTH - 1) {
        t->literal[i++] = ch;
        ch = l->input[l->pos++];
      }
      t->literal[i++] = '\0';
      t->type = TOKEN_INT;

      // return last character to input
      l->pos--;
    } else {
      t->type = TOKEN_ILLEGAL;
      t->literal[0] = ch;
    }
    break;
  }

  case '\0':
    t->type = TOKEN_EOF;
    t->literal[0] = '\0';
    return -1; // signal DONE
    break;
  }

  return 1;
}

struct lexer new_lexer(const char *input) {
  return (struct lexer){
      .input = input,
      .pos = 0,
      .cur_lineno = 1,
  };
}