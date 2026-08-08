/*===================================================================
                          error.h

                    (c)2026 SCXRPIUS.dev

              The Apollo error Compile Time library.
      Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/




#ifndef APOLLO_ERROR_H
#define APOLLO_ERROR_H

#include "token.h"
#include <stdbool.h>

typedef enum {
  SYNTAXERROR,
  ASSIGNMENTERROR,
  REFERROR,
  TYPEERROR,
  LEXERROR,
  SEMANTICERROR,
} ErrorType;

typedef struct Error {
  Token token;
  ErrorType type;
  char *message;
  char *got;
  int line;
} Error;

// Error stack

typedef struct ErrorStack {
  Error **data;
  size_t size;
  size_t capacity;

} ErrorStack;

/**
 * Initializes a new error stack.
 * @param s The error stack to initialize.
 */
void errorStack_init(ErrorStack *s);

/**
 * Pushes a new error onto the error stack.
 * @param s The error stack.
 * @param e The error object to push.
 * @return True if successful, false otherwise.
 */
bool errorStack_push(ErrorStack *s, Error *e);

/**
 * Pops the top error from the error stack.
 * @param s The error stack.
 * @return The popped error object.
 */
Error *errorStack_pop(ErrorStack *s);

/**
 * Reallocates memory for the error stack to increase capacity.
 * @param s The error stack.
 */
void realloc_errorStack(ErrorStack *s);

/**
 * Iterates through the error stack and prints all accumulated errors.
 * @param s The error stack.
 */
void errorStack_seek(ErrorStack *s);

/**
 * Frees the memory allocated for the error stack and its contents.
 * @param s The error stack.
 */
void errorStack_free(ErrorStack *s);

/**
 * Reports a new error by adding it to the parser's error stack.
 * @param parser The parser context.
 * @param errorMessage The description of the error.
 * @param type The type of the error (e.g., SYNTAXERROR).
 */
void error(Parser *parser, const char *errorMessage, ErrorType type);

#endif
