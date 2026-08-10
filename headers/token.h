/*===================================================================
                         token.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Token Compile Time library.
      Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/


#ifndef APOLLO_TOKEN_H
#define APOLLO_TOKEN_H

#include "defs.h"
#include "functions.h"
#include <ctype.h>
#include <stdbool.h>
#include <string.h>


typedef struct ErrorStack ErrorStack;


/*-------------TOKEN TYPES-------------*/
typedef enum {
  TOKEN_FXN, //----------------------------------------------------------> fxn [0]
  TOKEN_EOF, //----------------------------------------------------------> \0 [1]
  TOKEN_RUN, //----------------------------------------------------------> run() [2]
  TOKEN_VOID, //---------------------------------------------------------->
              //(void) [3]
  TOKEN_IDENTIFIER, //---------------------------------------------------------->
                    //undefined tokens [4]
  TOKEN_ARROW,  //----------------------------------------------------------> -> [5]
  TOKEN_STRING, //---------------------------------------------------------->
                //"Apollo" [6]
  TOKEN_LPARETH, //----------------------------------------------------------> ( [7]
  TOKEN_RPARETH, //----------------------------------------------------------> ) [8]
  TOKEN_LBRACE,  //----------------------------------------------------------> { [9]
  TOKEN_RBRACE,  //----------------------------------------------------------> } [10]
  TOKEN_SEMICOLON, //---------------------------------------------------------->
                   //; [11]
  TOKEN_INT,  //----------------------------------------------------------> e.g:
              //45 [12]
  TOKEN_DOUB, //---------------------------------------------------------->
              //e.g: 45.00000 [13]
  TOKEN_FLOAT, //---------------------------------------------------------->
               //e.g.: 45.000000 [14]
  TOKEN_ADD,   //----------------------------------------------------------> + [15]
  TOKEN_MUL,   //----------------------------------------------------------> * [14]
  TOKEN_SUB,   //----------------------------------------------------------> - [13]
  TOKEN_DIV,   //----------------------------------------------------------> / [14]
  TOKEN_MOD,   //----------------------------------------------------------> % [15]
  TOKEN_INC,   //----------------------------------------------------------> ++ [16]
  TOKEN_DEC,   //----------------------------------------------------------> -- [17]
  TOKEN_AEQ,   //----------------------------------------------------------> += [18]
  TOKEN_SEQ,   //----------------------------------------------------------> -= [19]
  TOKEN_MEQ,   //----------------------------------------------------------> *= [20]
  TOKEN_DEQ,   //----------------------------------------------------------> /= 
  TOKEN_PEQ,   //----------------------------------------------------------> %=
  //TOKEN_PRINTLN, //---------------------------------------------------------->println
  TOKEN_VAR, //----------------------------------------------------------> var
  TOKEN_ASSIGN, //----------------------------------------------------------> =
  TOKEN_BOOL,   // ---------------------------------------------------------->true/false
  TOKEN_NULL,   // ---------------------------------------------------------->
                // null
  TOKEN_EQT,    //----------------------------------------------------------> ==
  TOKEN_NEQ,    //----------------------------------------------------------> !=
  TOKEN_GT,     //----------------------------------------------------------> >
  TOKEN_ST,     //----------------------------------------------------------> <
  TOKEN_GE,     //----------------------------------------------------------> >=
  TOKEN_SE,     //----------------------------------------------------------> <=
  TOKEN_AND,    //---------------------------------------------------------->
                //and/&&
  TOKEN_OR, //----------------------------------------------------------> or/||
  TOKEN_IF, //----------------------------------------------------------> if()
  TOKEN_ELIF, //---------------------------------------------------------->elif()
  TOKEN_ELSE,  //---------------------------------------------------------->
               //else{}
  TOKEN_WHILE, //---------------------------------------------------------->
               //while()
  TOKEN_FOR, //----------------------------------------------------------> for()

  DECLARE_INT,
  DECLARE_STR,
  DECLARE_BOOL,
  DECLARE_FLOAT,
  TOKEN_COMMA,
  TOKEN_RETURN,
  TOKEN_CHAR,

} TokenType;





/*-------------DEFINING THE TOKEN-------------*/
typedef struct {
  TokenType type;    // Token type
  const char *start; // RAM Address of Token
  int length;        //
  int line;
} Token;

/*Tells compiler that the variable exists in another file,
  Check lexer.c
*/
extern const char *TokenNames[];

// ACTS AS A POINTER THAT KEEPS TRACK OF POSITION IN SOURCE CODE
typedef struct {
  const char *current;
  int line;
  int column;
  int scope_level;
  Fxn *fxn;
  ErrorStack *errors;
} Lexer;

/**
 * Advances the lexer and returns the next token from the source code.
 * @param lexer The lexer context tracking the current position and state.
 * @return The next token extracted from the source.
 */
Token next_token(Lexer *lexer);

__attribute__((always_inline)) void lex_error(Lexer *lexer, const char *message, const char *got);

// Tracks the internal stream state of parser.
typedef struct {
  Lexer *lexer;   // takes lexer details (current character, current line)
  Token current;  // takes details of the current token
  Token previous; // takes details of the previous token
  ErrorStack *error;
} Parser;

#endif
