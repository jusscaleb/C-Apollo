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

void errorStack_init(errorStack *s);

bool errorStack_push(errorStack *s, Error *e);

Error* errorStack_pop(errorStack *s);


void realloc_errorStack(errorStack *s);
void errorStack_seek(errorStack *s);
void errorStack_free(errorStack *s);

void error(Parser *parser, const char *errorMessage, ErrorType type);

#endif
