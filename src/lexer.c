/*--------------------------------------------------------------------------------

                        CHARACTER GROUPER :)

---------------------------------------------------------------------------------*/

#include "../headers/error.h"
#include "../headers/token.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Defining the TokenNames in token.h
const char *TokenNames[] = {
    "FXN",    "EOF",    "RUN",    "VOID",   "IDENTIFIER", "ARROW", "STRING",
    "LPAREN", "RPAREN", "LBRACE", "RBRACE", "SEMICOLON",  "INT",   "DOUB",
    "FLOAT",  "ADD",    "MUL",    "SUB",    "DIV",        "MOD",   "INC",
    "DEC",    "AEQ",    "SEQ",    "MEQ",    "DEQ",        "PEQ",   "PRINTLN",
    "VAR",    "ASSIGN", "BOOL",   "NULL",   "EQT",        "NEQ",   "GT",
    "ST",     "GE",     "SE",     "AND",    "OR",         "IF",    "ELIF",
    "ELSE",   "WHILE",  "FOR"};

// FOR ERROR HANDLING
void lex_error(Lexer *lexer, const char *message, const char *got) {
  Error *e = (Error *)alloc_space(1, sizeof(Error));
  e->message = _strdup(message);
  e->token.line = lexer->line;
  e->type = LEXERROR;
  e->got = _strdup(got);

  errorStack_push(lexer->errors, e);
}

/**
 *Checks if the keyword is reserved.
 *If not it is a user-defined keyword
 */
static TokenType check_keyword(const char *start, int length) {
  if (length == 3 && strncmp(start, "fxn", 3) == 0)
    return TOKEN_FXN;

  if (length == 3 && strncmp(start, "run", 3) == 0)
    return TOKEN_RUN;
  if (length == 4 && strncmp(start, "void", 4) == 0)
    return TOKEN_VOID;
  if (length == 7 && strncmp(start, "println", 7) == 0)
    return TOKEN_PRINTLN;
  if (length == 3 && strncmp(start, "var", 3) == 0)
    return TOKEN_VAR;

  // BOOLEANS
  if (length == 4 && strncmp(start, "true", 4) == 0)
    return TOKEN_BOOL;
  if (length == 5 && strncmp(start, "false", 5) == 0)
    return TOKEN_BOOL;

  // NULL VALUE
  if (length == 4 && strncmp(start, "null", 4) == 0)
    return TOKEN_NULL;

  // Logical Conditions
  if (length == 3 && strncmp(start, "and", 3) == 0)
    return TOKEN_AND;

  if (length == 2 && strncmp(start, "or", 2) == 0)
    return TOKEN_OR;

  // Conditions
  if (length == 2 && strncmp(start, "if", 2) == 0)
    return TOKEN_IF;
  if (length == 4 && strncmp(start, "else", 4) == 0)
    return TOKEN_ELSE;
  if (length == 4 && strncmp(start, "elif", 4) == 0)
    return TOKEN_ELIF;

  // Loops
  if (length == 5 && strncmp(start, "while", 5) == 0)
    return TOKEN_WHILE;

  if (length == 3 && strncmp(start, "for", 3) == 0)
    return TOKEN_FOR;

  return TOKEN_IDENTIFIER;
}

Token next_token(Lexer *lexer) {
  // Loops through to consume and ignore spaces, tabs and carriage returns.
  while (*lexer->current == ' ' || *lexer->current == '\r' ||
         *lexer->current == '\t' || *lexer->current == '\n') {
    if (*lexer->current == '\n') {
      lexer->line++;
    } // move to the next line.
    lexer->current++;
    lexer->column++; // move pointer one character forward.
  }

  // set start to current character...
  const char *start = lexer->current;

  // HIT NULL CHARACTER/ END OF PROGRAM...
  if (*lexer->current == '\0') {
    Token token = {TOKEN_EOF, start, 0, lexer->line};
    lexer->column = 0;
    return token;
  }

  // read the current character and move on;
  char c = *lexer->current++;
  lexer->column++;

  // using a switch to check single characters
  switch (c) {
  case '}': {
    Token token = {TOKEN_RBRACE, start, 1, lexer->line};
    return token;
  }

  case '{': {
    Token token = {TOKEN_LBRACE, start, 1, lexer->line};
    return token;
  }

  case '(': {
    Token token = {TOKEN_LPARETH, start, 1, lexer->line};
    return token;
  }

  case ')': {
    Token token = {TOKEN_RPARETH, start, 1, lexer->line};
    return token;
  }

  case ';': {
    Token token = {TOKEN_SEMICOLON, start, 1, lexer->line};
    lexer->column = 0;
    return token;
  }

  case '-': {
    if (*lexer->current == '>') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_ARROW, start, 2, lexer->line};
      return token;
    } else if (*lexer->current == '-') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_DEC, start, 2, lexer->line};
      return token;

    } else if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_SEQ, start, 2, lexer->line};
      return token;
    }

    Token token = {TOKEN_SUB, start, 1, lexer->line};
    return token;
  }

  case '+':
    if (*lexer->current == '+') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_INC, start, 2, lexer->line};
      return token;

    } else if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_AEQ, start, 2, lexer->line};
      return token;
    }

    Token token = {TOKEN_ADD, start, 1, lexer->line};
    return token;

  case '*': {

    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_MEQ, start, 2, lexer->line};
      return token;
    }
    Token token = {TOKEN_MUL, start, 1, lexer->line};
    return token;
  }

  case '/': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_DEQ, start, 2, lexer->line};
      return token;
    }
    Token token = {TOKEN_DIV, start, 1, lexer->line};
    return token;
  }

  case '%': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_PEQ, start, 2, lexer->line};
      return token;
    }

    Token token = {TOKEN_MOD, start, 1, lexer->line};
    return token;
  }
  case '#': {
    while (*lexer->current != '\0' && *lexer->current != '\n') {
      lexer->current++;
      lexer->column++;
    }

    return next_token(lexer);
  }

  case '=': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_EQT, start, 2, lexer->line};
      return token;
    }
    Token token = {TOKEN_ASSIGN, start, 1, lexer->line};
    return token;
  }

  case '!': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_NEQ, start, 2, lexer->line};
      return token;
    }

    lex_error(lexer, "Unrecognized Token. Did you mean '!='?", "!");
    Token token = {TOKEN_EOF, start, 1, lexer->line};
    return token;
  }

  case '>': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_GE, start, 2, lexer->line};
      return token;
    }

    Token token = {TOKEN_GT, start, 1, lexer->line};
    return token;
  }

  case '<': {
    if (*lexer->current == '=') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_SE, start, 2, lexer->line};
      return token;
    }

    Token token = {TOKEN_ST, start, 1, lexer->line};
    return token;
  }

  case '&': {
    if (*lexer->current == '&') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_AND, start, 3, lexer->line};
      return token;
    }

    lex_error(lexer, "Could not recognize token. Did you mean '&&' ?", "&");
    Token token = {TOKEN_EOF, start, 1, lexer->line};
    return token;
  }

  case '|': {
    if (*lexer->current == '|') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_OR, start, 2, lexer->line};
      return token;
    }

    lex_error(lexer, "Could not recognize token. Did you mean '||'?", "|");
    Token token = {TOKEN_EOF, start, 1, lexer->line};
    return token;
  }
  }

  // HANDLE STRINGS
  if (c == '"') {
    // Loops until it reaches closing quotes or \0.
    while (*lexer->current != '\0') {
      if (*lexer->current == '\n')
        lexer->line++;

      if (*lexer->current == '"') {
        break;
      }

      // Checks for escaped characters.
      if (*lexer->current == '\\') {
        lexer->current++;
        lexer->column++;

        // If it reaches \0 that means it's an unterminated string.
        if (*lexer->current == '\0') {
          int len = (int)(lexer->current - start);
          char *temp = malloc(len + 1);
          snprintf(temp, len + 1, "%.*s", len, start);
          lex_error(lexer, "Unterminated String", temp);
          free(temp);
          Token token = {TOKEN_EOF, start, len, lexer->line};
          return token;
        }

        lexer->current++;
        lexer->column++;
        continue;
      }

      lexer->current++;
      lexer->column++;
    }

    // If it reaches \0 that means it's an unterminated string.
    if (*lexer->current == '\0') {
      int len = (int)(lexer->current - start);
      char *temp = malloc(len + 1);
      snprintf(temp, len + 1, "%.*s", len, start);
      lex_error(lexer, "Unterminated String", temp);
      free(temp);
      Token token = {TOKEN_EOF, start, len, lexer->line};
      return token;
    }

    int length = (int)(lexer->current - start);
    lexer->current++;
    lexer->column++;

    Token token = {TOKEN_STRING, start, length, lexer->line};
    return token;
  }

  // Checking Int
  if (isdigit((unsigned char)c)) {
    set is_float = false;

    while (isdigit((unsigned char)*lexer->current)) {
      lexer->current++;
      lexer->column++;
    }

    if (*lexer->current == '.') {
      is_float = true;
      lexer->current++;
      lexer->column++;

      if (!isdigit((unsigned char)*lexer->current)) {
        int len = (int)(lexer->current - start);
        char *temp = malloc(len + 1);
        snprintf(temp, len + 1, "%.*s", len, start);
        lex_error(lexer, "Expected value int after '.' ", temp);
        free(temp);
      }
    }
    while (isdigit((unsigned char)*lexer->current)) {
      lexer->current++;
      lexer->column++;
    }

    int length = (int)(lexer->current - start);

    TokenType type = (is_float) ? TOKEN_FLOAT : TOKEN_INT;
    Token token = {type, start, length, lexer->line};
    return token;
  }

  // Catering for other types of Keywords
  if (isalpha(c) || c == '_') {
    // Could my_number_2...
    while (isalnum((unsigned char)*lexer->current) || *lexer->current == '_') {
      lexer->current++;
      lexer->column++;
    }

    // Loop will likely end at \n, \0 or ;.
    int length = (int)(lexer->current - start);

    // check for the type of token
    TokenType type = check_keyword(start, length);
    Token token = {type, start, length, lexer->line};

    return token;
  }

  char temp[2] = {c, '\0'};
  lex_error(lexer, "Undefined string.", temp);
  Token token = {TOKEN_EOF, start, 1, lexer->line};
  return token;
}
