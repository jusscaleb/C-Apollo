#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "token.h"


typedef bool set;

//Defining the TokenNames in token.h
const char* TokenNames[] = {
    "EOF", "FXN", "RUN", "VOID", "IDENTIFIER", 
    "STRING", "ARROW", "LPAREN", "RPAREN", 
    "LBRACE", "RBRACE", "SEMICOLON", "ARROW"
};

//ACTS AS A POINTER THAT KEEPS TRACK OF POSITION IN SOURCE CODE
typedef struct {
    const char* current;
    int line;
} Lexer;

//FOR ERROR HANDLING
void lex_error(int line, const char* message) {
    fprintf(stderr, "Lexical Error (Line %d): %s\n", line, message);
    exit(1);
}


/**
*Checks if the keyword is reserved
*If not it is a user-defined keyword
*/
static TokenType check_keyword(const char* start, int length){
    if(length == 3 && strncmp(start, "fxn", 3)) return TOKEN_FXN;
    if(length == 3 && strncmp(start, "run", 3)) return TOKEN_RUN;
    if(length == 3 && strncmp(start, "void", 4)) return TOKEN_VOID;

    return TOKEN_IDENTIFIER;
}


Token next_token(Lexer* lexer){
    //Loops through to consume and ignore spaces, tabs and carriage returns.
    set skip =*lexer->current == ' ' || *lexer->current == '\r' || *lexer->current == '\t' || *lexer->current == '\n';
    set newLine = *lexer->current == '\n';

    while(skip){
        if(newLine){lexer->line++;} //move to the next line.
        lexer->current++; //move pointer one character forward.

    }

    //set start to current character...
    const char* start = lexer->current;

    //HIT NULL CHARACTER/ END OF PROGRAM...
    if(*lexer->current == '\0'){
        Token token = {TOKEN_EOF, start, 0, lexer->line};
        return token;
    }

    //reac the current character and move on;

    char c = *lexer->current++;

    //using a switch to check single characters
    switch(c){
        case ')':
        {
            Token token = {TOKEN_RBRACE, start, 1, lexer->line};
            return token;
        }

        case '(':
        {
            Token token = {TOKEN_LBRACE, start, 1, lexer->line};
            return token;
        }

        case '{':
        {
            Token token = {TOKEN_LPARETH, start, 1, lexer->line};
            return token;
        }

        case '}':
        {
            Token token = {TOKEN_RPARETH, start, 1, lexer->line};
            return token;

        }

        case ';':
        {
            Token token = {TOKEN_SEMICOLON, start, 1, lexer->line};
            return token;
        }

        case '-':
            {
                if(*lexer -> current == '>'){
                    lexer->current++;
                    
                    Token token = {TOKEN_ARROW, start, 2, lexer->line};
                    return token;
                }

                lex_error(lexer->line, "Syntax Error: expected '>' after '-'.");
            }

    }

    //HANDLE STRINGS
    if(*lexer->current == '"'){
        //Loops until it reaches closing quotes or \0.
        while(*lexer->current != '"' && *lexer->current != '\0'){
            if(*lexer->current == '\n')lexer->line++;

            lexer->current++;
        }

        //If it reaches \0 that means it's an unterminated string.
        if(*lexer->current=='\0') lex_error(lexer->line, "Syntax Error: Unterminated String");

    int length = (int)(lexer->current - start);
    lexer->current++;

    Token token = {TOKEN_STRING, start, length, lexer->line};
    return token;

    }

    //Catering for other types of Keywords
    if(isalpha(*lexer->current) || c == '_'){
        //Could my_number_2...
        while(isalnum(*lexer->current) || c == '_'){
            lexer->current++;
        }

        //Loop will likely end at \n, \0 or ;.
        int length = (int)(lexer->current - start);

        //check for the type of token
        TokenType type = check_keyword(start,length);
        Token token = {type, start, length, lexer->line};

        return token;
    }

    lex_error(lexer->line, "Undefined string.");
    exit(1);
}
