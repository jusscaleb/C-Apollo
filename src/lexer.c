/*--------------------------------------------------------------------------------

                        CHARACTER GROUPER :)

---------------------------------------------------------------------------------*/

#include "../headers/error.h"
#include "../headers/token.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

// Defining the TokenNames in token.h
const char *TokenNames[] = {
    "Fxn",    "EOF",    "run",    "void",   "IDENTIFIER", "->", "STRING",
    "(", ")", "{", "}", ";",   "INT",   "DOUBLE",
    "FLOAT",  "+",    "*",    "-",    "/",    "%",   "++",
    "--",    "+=",   "-=",   "*=",   "/=",   "%=",   "println",
    "var",    "=",      "bool",   "null",   "==",        "!=",   ">",
    "<",     ">=",     "<=",     "and",    "or",         "if",    "elif",
    "else",   "while",  "for",    "int",    "str",        "bool",  "float",
    ","};

// FOR ERROR HANDLING
 __attribute__((always_inline)) void lex_error(Lexer *lexer, const char *message, const char *got) {
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

 //TO BE MODIFIED FOR OPTIMIZATION.
__attribute__((always_inline)) static TokenType check_keyword(const char *start, int length) {
  //check length 3
  TokenType token = TOKEN_IDENTIFIER;
  switch(length){
    case 2:
      if(start[0] == 'o' && start[1] == 'r') token = TOKEN_OR;
      if(start[0] == 'i' && start[1] == 'f') token = TOKEN_IF;
      break;
    
    case 3:
      if(memcmp(start, "fxn", 3) == 0) token = TOKEN_FXN;
      if(memcmp(start, "run", 3) == 0) token = TOKEN_RUN;
      if(memcmp(start, "var", 3) == 0) token = TOKEN_VAR;
      if(memcmp(start, "and", 3) == 0) token = TOKEN_AND;
      if(memcmp(start, "int", 3) == 0) token = DECLARE_INT;
      if(memcmp(start, "for", 3) == 0) token = TOKEN_FOR;
      if(memcmp(start, "str", 3) == 0) token = DECLARE_STR;
      break;

    case 4:
      if(memcmp(start, "void", 4) == 0) token = TOKEN_VOID;
      if(memcmp(start, "true", 4) == 0) token = TOKEN_BOOL;
      if(memcmp(start, "else", 4) == 0) token = TOKEN_ELSE;
      if(memcmp(start, "elif", 4) == 0) token = TOKEN_ELIF;
      if(memcmp(start, "null", 4) == 0) token = TOKEN_NULL;
      if(memcmp(start, "bool", 4) == 0) token = DECLARE_BOOL;
      break;
    
    case 5:
      if(memcmp(start, "false", 5) == 0) token = TOKEN_BOOL;
      if(memcmp(start, "while", 5) == 0) token = TOKEN_WHILE;
      if(memcmp(start, "float", 5) == 0) token = DECLARE_FLOAT;
      break;
    
    case 6:
      if(memcmp(start, "return", 6) == 0) token = TOKEN_RETURN;
      break;
      
  }

  return token;
}

Token next_token(Lexer *lexer) {
  while (*lexer->current == ' ' || *lexer->current == '\r' ||
         *lexer->current == '\t' || *lexer->current == '\n') {
    if (*lexer->current == '\n') {
      lexer->line++;
    } 
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
    lexer->scope_level--;
    // lexer->fxn = lexer->fxn->parent_fxn;
    return token;
  }

  case '{': {
    Token token = {TOKEN_LBRACE, start, 1, lexer->line};
    lexer->scope_level++;
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
    }if (*lexer->current == '-') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_DEC, start, 2, lexer->line};
      return token;

    }if (*lexer->current == '=') {
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
  case ',': {
    Token token = {TOKEN_COMMA, start, 1, lexer->line};
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
          char temp[len + 1];
          memcpy(temp, start, len);
          temp[len] = '\0'; 
          //snprintf(temp, len + 1, "%.*s", len, start);
          lex_error(lexer, "Unterminated String", temp);
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
      char temp[len + 1];
      memcpy(temp, start, len);
      temp[len] = '\0';
      //snprintf(temp, len + 1, "%.*s", len, start);
      lex_error(lexer, "Unterminated String", temp);
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
        char temp[len + 1];
        //snprintf(temp, len + 1, "%.*s", len, start);
        memcpy(temp, start, len);
        temp[len] = '\0';
        lex_error(lexer, "Expected value int after '.' ", temp);
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
