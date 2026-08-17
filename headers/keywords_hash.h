#ifndef APOLLO_KEYWORDS_HASH_H
#define APOLLO_KEYWORDS_HASH_H

#include <string.h>
#include "token.h"

struct KeywordEntry {
  const char *name;
  TokenType type;
};

static inline const struct KeywordEntry *in_word_set(const char *str, unsigned int len) {
  static const struct KeywordEntry wordlist[] = {
    {"fxn", TOKEN_FXN},
    {"run", TOKEN_RUN},
    {"void", TOKEN_VOID},
    {"var", TOKEN_VAR},
    {"true", TOKEN_BOOL},
    {"false", TOKEN_BOOL},
    {"null", TOKEN_NULL},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"if", TOKEN_IF},
    {"elif", TOKEN_ELIF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},
    {"int", DECLARE_INT},
    {"str", DECLARE_STR},
    {"bool", DECLARE_BOOL},
    {"float", DECLARE_FLOAT},
    {"return", TOKEN_RETURN}
  };

  static const int count = sizeof(wordlist) / sizeof(wordlist[0]);

  if (len < 2 || len > 6) return NULL;

  for (int i = 0; i < count; i++) {
    if (wordlist[i].name[0] == str[0] &&
        wordlist[i].name[1] == str[1] &&
        memcmp(wordlist[i].name, str, len) == 0 &&
        wordlist[i].name[len] == '\0') {
      return &wordlist[i];
    }
  }

  return NULL;
}

#endif
