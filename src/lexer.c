#include "../headers/error.h"
#include "../headers/keywords_hash.h"
#include "../headers/token.h"

const char *TokenNames[] = {
    "Fxn",        "EOF",        "run",       "void",       "IDENTIFIER",
    "->",         "STRING",     "(",         ")",          "{",
    "}",          ";",          "INT",       "DOUBLE",     "FLOAT",
    "+",          "*",          "-",         "/",          "%",
    "++",         "--",         "+=",        "-=",         "*=",
    "/=",         "%=",         "var",       "=",          "bool",
    "null",       "==",         "!=",        ">",          "<",
    ">=",         "<=",         "and",       "or",         "if",
    "elif",       "else",       "while",     "for",        "int",
    "str",        "bool",       "float",     "char",       "int*",
    "char*",      "bool*",      "float*",    "str*",       ",",
    "return",     "char_lit",   "&","*","...", "@"
};

// FOR ERROR HANDLING
__attribute__((always_inline)) void lex_error(Lexer *lexer, const char *message,
                                              const char *got) {
  Error *e = (Error *)alloc_space(1, sizeof(Error));
  e->message = _strdup(message);
  e->token.line = lexer->line;
  e->type = LEXERROR;
  e->got = _strdup(got);

  errorStack_push(lexer->errors, e);
}

/**
 *Checks if the keyword is reserved using gperf pre-hashed lookup table.
 *If not it is a user-defined keyword
 */
__attribute__((always_inline)) TokenType check_keyword(const char *start,
                                                       int length) {
  const struct KeywordEntry *entry = in_word_set(start, length);
  return entry ? entry->type : TOKEN_IDENTIFIER;
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
    if (_IS_DIGIT_((unsigned char)*lexer->current)) {
      return check_number(lexer, start);
    }

    if (*lexer->current == '>') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_ARROW, start, 2, lexer->line};
      return token;
    }
    if (*lexer->current == '-') {
      lexer->current++;
      lexer->column++;
      Token token = {TOKEN_DEC, start, 2, lexer->line};
      return token;
    }
    if (*lexer->current == '=') {
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
    if (*lexer->current == '/') {
      while (*lexer->current != '\0' && *lexer->current != '\n') {
        lexer->current++;
        lexer->column++;
      }
      return next_token(lexer);
    }
    if (*lexer->current == '*') {
      lexer->current++;
      lexer->column++;

      while (*lexer->current != '\0') {
        if (*lexer->current == '*' && *(lexer->current + 1) == '/') {
          lexer->current += 2;
          lexer->column += 2;
          return next_token(lexer);
        }

        if (*lexer->current == '\n') {
          lexer->line++;
          lexer->column = 1;
        } else {
          lexer->column++;
        }
        lexer->current++;
      }
      lex_error(lexer, "Unterminated block comment.", "");
      return next_token(lexer);
    }
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

    //lex_error(lexer, "Could not recognize token. Did you mean '&&' ?", "&");
    Token token = {TOKEN_REF, start, 1, lexer->line};
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
  case '.': {
    if (*lexer->current == '.' && *(lexer->current + 1) == '.') {
      lexer->current += 2;
      lexer->column += 2;
      Token token = {TOKEN_VARARGS, start, 3, lexer->line};
      return token;
    }

    lex_error(lexer, "Could not recognize token. Did you mean '...'?", ".");
    Token token = {TOKEN_EOF, start, 1, lexer->line};
    return token;
  }
  case '@':{
    Token token = {TOKEN_SIGIL, start, 1, lexer->line};
    return token;
  }



  }

  // HANDLE STRINGS
  if (c == '"') {
    return check_string_or_char(lexer, start, true);
  }

  // HANDLE CHARACTERS
  if (c == '\'') {
    return check_string_or_char(lexer, start, false);
  }

  // Checking Int
  if (_IS_DIGIT_((unsigned char)c)) {
    return check_number(lexer, start);
  }

  // Catering for other types of Keywords
  if (_IS_ALPHA_(c) || c == '_') {
    // Could my_number_2...
    while (_IS_ALNUM_((unsigned char)*lexer->current) ||
           *lexer->current == '_') {
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

__attribute__((always_inline)) Token check_number(Lexer *lexer,
                                                  const char *start) {
  bool is_float = false;
  while (_IS_DIGIT_((unsigned char)*lexer->current)) {
    lexer->current++;
    lexer->column++;
  }

  if (*lexer->current == '.') {
    is_float = true;
    lexer->current++;
    lexer->column++;

    if (!_IS_DIGIT_((unsigned char)*lexer->current)) {
      int len = (int)(lexer->current - start);
      char temp[len + 1];
      memcpy(temp, start, len);
      temp[len] = '\0';
      lex_error(lexer, "Expected value int after '.' ", temp);
    }
  }
  while (_IS_DIGIT_((unsigned char)*lexer->current)) {
    lexer->current++;
    lexer->column++;
  }

  int length = (int)(lexer->current - start);

  TokenType type = (is_float) ? TOKEN_FLOAT : TOKEN_INT;
  Token token = {type, start, length, lexer->line};
  return token;
}

__attribute__((always_inline)) Token check_string_or_char(Lexer *lexer,
                                                          const char *start,
                                                          bool is_str) {

  const char TERMINATING_STRING = is_str ? '"' : '\'';
  const char *err_msg =
      is_str ? "Unterminated String Literal" : "Unterminated Character Literal";
  TokenType t = is_str ? TOKEN_STRING : TOKEN_CHAR;

  while (*lexer->current != '\0') {
    if (*lexer->current == '\n')
      lexer->line++;

    if (*lexer->current == TERMINATING_STRING) {
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
        lex_error(lexer, err_msg, temp);
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

  if (*lexer->current == '\0') {
    int len = (int)(lexer->current - start);
    char temp[len + 1];
    memcpy(temp, start, len);
    temp[len] = '\0';
    lex_error(lexer, err_msg, temp);
    Token token = {TOKEN_EOF, start, len, lexer->line};
    return token;
  }

  int length = (int)(lexer->current - start);

  if (!is_str) {
    if (length + 1 < 3 || length + 1 > 4) {
      lex_error(lexer, "Character literal must contain exactly one character",
                start);
    }
  }

  lexer->current++;
  lexer->column++;

  Token token = {t, start, length, lexer->line};
  return token;
}