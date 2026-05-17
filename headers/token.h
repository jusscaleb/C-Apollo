/*
Where the tokens used in the compiler are stored.
*/
#ifndef APOLLO_TOKEN_H
#define APOLLO_TOKEN_H

#include <stdio.h>
#include "defs.h"




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
    TOKEN_INT,
    TOKEN_DOUB,
    TOKEN_FLOAT,
    TOKEN_ADD
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

//ACTS AS A POINTER THAT KEEPS TRACK OF POSITION IN SOURCE CODE
typedef struct {
    const char* current;
    int line;
} Lexer;

//Called by parser.c
Token next_token(Lexer* lexer);

//Tracks the internal stream state of parser.

typedef struct{
    Lexer* lexer; //takes lexer details (current character, current line)
    Token current; //takes details of the current token
    Token previous; //takes details of the previous token
}Parser;

#endif

