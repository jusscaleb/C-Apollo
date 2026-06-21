#include "../headers/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ErrorToken[] = {"Syntax Error", "Assignment Error", "Reference Error",
                            "Type Error", "Lexical Error", "Semantic Error"};

void errorStack_init(errorStack *s) {

  s->size = 0;
  s->capacity = 0;
  s->data = NULL;
}

bool errorStack_push(errorStack *s, Error *e) {
  realloc_errorStack(s);
  s->data[s->size] = e;
  s->size++;
  return true;
}

Error *errorStack_pop(errorStack *s) {
  if (s->size == 0) {
    return NULL;
  }
  s->size--;
  Error *e = s->data[s->size];
  return e;
}

void errorStack_seek(errorStack *s) {
  while (s->size > 0) {
    Error *e = errorStack_pop(s);
    fprintf(stderr, "[%s] %s Line [%d]", ErrorToken[e->type], e->message,
            e->token.line);

    if (e->got != NULL && strlen(e->got) > 0)
      fprintf(stderr, " (Found: %s)", e->got);

    fprintf(stderr, "\n");
  }
}

void error(Parser *parser, const char *errorMessage, ErrorType type) {
  Error *e = (Error *)alloc_space(1, sizeof(Error));
  e->type = type;
  e->message = _strdup(errorMessage);
  e->token = parser->current;

  int len = parser->current.length;
  e->got = alloc_space(len + 1, sizeof(char));
  if (parser->current.start != NULL) {
      sprintf(e->got, "%.*s", len, parser->current.start);
  } else {
      e->got[0] = '\0';
  }
  e->got[len] = '\0';

  errorStack_push(parser->lexer->errors, e);
}