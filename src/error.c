#include "../headers/error.h"
#include <string.h>



const char *ErrorToken[] = {"Syntax Error",    "Assignment Error",
                            "Reference Error", "Type Error",
                            "Lexical Error",   "Semantic Error"};

void errorStack_init(ErrorStack *s) {

  s->size = 0;
  s->capacity = 0;
  s->data = NULL;
}

bool errorStack_push(ErrorStack *s, Error *e) {
  realloc_errorStack(s);
  s->data[s->size] = e;
  s->size++;
  return true;
}
__attribute__((always_inline)) Error *errorStack_pop(ErrorStack *s) {
  if (s->size == 0) {
    return NULL;
  }
  s->size--;
  Error *e = s->data[s->size];
  return e;
}

void errorStack_seek(ErrorStack *s) {
  while (s->size > 0) {
    Error *e = errorStack_pop(s);
    fprintf(stderr, "[%s] %s Line [%d]", ErrorToken[e->type], e->message,
            e->line);

    if (e->got != NULL && strlen(e->got) > 0)
      fprintf(stderr, " (Found: %s)", e->got);

    fprintf(stderr, "\n");
    fflush(stderr);
  }
  fflush(stderr);
}

void error(Parser *parser, const char *errorMessage, ErrorType type) {
  Error *e = (Error *)alloc_space(1, sizeof(Error));
  e->type = type;
  e->message = _strdup(errorMessage);
  e->token = parser->current;
  e->line = parser->current.line-1;

  int len = (parser->current.length > 0) ? parser->current.length : 0;
  e->got = alloc_space(len + 1, sizeof(char));
  if (parser->current.start != NULL && len > 0) {
    memcpy(e->got, parser->current.start, len);
    e->got[len] = '\0';
  } else {
    e->got[0] = '\0';
  }
  e->got[len] = '\0';

  errorStack_push(parser->lexer->errors, e);
}
