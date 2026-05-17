/*
Where the tokens used in the compiler are stored.
*/

#include <stdio.h>

#ifndef APOLLO_TOKEN_H
#define APOLLO_TOKEN_H


/*-------------TOKEN TYPES-------------*/
typedef enum{
    TOKEN_FXN, //fxn
    TOKEN_EOF,
    TOKEN_RUN, //run()
    TOKEN_VOID, //void return type
    TOKEN_IDENTIFIER, //e.g.: println
    TOKEN_ARROW, //->
    TOKEN_STRING, //string datatype
    TOKEN_LPARETH, //{
    TOKEN_RPARETH, //}
    TOKEN_LBRACE, //(
    TOKEN_RBRACE, //)
    TOKEN_SEMICOLON, //;
    TOKEN_ERROR, 
}TokenType;


/*-------------DEFINING THE TOKEN-------------*/
typedef struct{
    TokenType type; //Token type
    const char* start; //RAM Address of Token
    int length; //
    int line;
}Token;

/*Tells compiler that the variable exists in another file,
  Check lexer.c
*/
extern const char* TokenNames[];

#endif

