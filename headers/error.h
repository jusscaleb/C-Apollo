#ifndef APOLLO_ERROR_H
#define APOLLO_ERROR_H


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "defs.h"
#include "token.h"




typedef enum{
    SYNTAXERROR,
    ASSIGNMENTERROR,
    REFERROR,
    TYPEERROR,
    LEXERROR,
    SEMANTICERROR,
}ErrorType;


typedef struct Error {
    Token token;
    ErrorType type;
    char* message;
    char* got;
}Error;

//Error stack

typedef struct errorStack {
    Error** data;
    size_t size;
    size_t capacity;

}errorStack;

/**
 * Initializes a new error stack.
 * @param s The error stack to initialize.
 */
void errorStack_init(errorStack *s);

/**
 * Pushes a new error onto the error stack.
 * @param s The error stack.
 * @param e The error object to push.
 * @return True if successful, false otherwise.
 */
bool errorStack_push(errorStack *s, Error *e);

/**
 * Pops the top error from the error stack.
 * @param s The error stack.
 * @return The popped error object.
 */
Error* errorStack_pop(errorStack *s);

/**
 * Reallocates memory for the error stack to increase capacity.
 * @param s The error stack.
 */
void realloc_errorStack(errorStack *s);

/**
 * Iterates through the error stack and prints all accumulated errors.
 * @param s The error stack.
 */
void errorStack_seek(errorStack *s);

/**
 * Frees the memory allocated for the error stack and its contents.
 * @param s The error stack.
 */
void errorStack_free(errorStack *s);

/**
 * Reports a new error by adding it to the parser's error stack.
 * @param parser The parser context.
 * @param errorMessage The description of the error.
 * @param type The type of the error (e.g., SYNTAXERROR).
 */
void error(Parser *parser, const char *errorMessage, ErrorType type);

#endif
