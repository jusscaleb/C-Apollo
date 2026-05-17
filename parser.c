#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "token.h"


//advance() move on to next token
static void advance(Parser* parser){
    parser->previous = parser->current;
    parser->current = next_token(parser->lexer);
    
}

//consume() checks if given token matches expected token.
static void consume(Parser* parser, TokenType type, const char* errorMessage){
    
    if(parser->current.type == type){
        advance(parser);
        return;
    }

    fprintf(stderr, "Apollo Syntax Error [Line %d]: %s\n", parser->current.line, errorMessage);
    fprintf(stderr, "Found: '%.*s' (Type: %s)\n", 
            parser->current.length, 
            parser->current.start, 
            TokenNames[parser->current.type]);
    
    exit(1);
}
//check the body statement
static void parse_body_statement(Parser* parser) {
    consume(parser, TOKEN_IDENTIFIER, "Expected function identifier statement inside block.");
    consume(parser, TOKEN_LPARETH, "Expected open parenthesis '(' for arguments.");
    consume(parser, TOKEN_STRING, "Expected string literal argument inside function call.");
    consume(parser, TOKEN_RPARETH, "Expected closing parenthesis ')' for arguments.");
    consume(parser, TOKEN_SEMICOLON, "Expected trailing semicolon ';' to terminate statement.");
}


//parse_block() checks the innard of that function.
static void parse_block(Parser* parser){
    consume(parser, TOKEN_LBRACE, "Expected open brace '{' to begin function block definition.");

    while(parser->current.type != TOKEN_RBRACE && parser->current.type != TOKEN_EOF){
        parse_body_statement(parser);

    }

    consume(parser, TOKEN_RBRACE, "Expected closing brace '}' to terminate block context.");
}

//parse_function check if the function is grammatically correct. (fxn run()->(void){})
void parse_function(Parser* parser){
    //verifies fxn run()
    consume(parser, TOKEN_FXN, "Expected function declaration keyword 'fxn'.");
    consume(parser, TOKEN_RUN, "Expected program entrypoint name 'run'.");
    consume(parser, TOKEN_LPARETH, "Expected parameter list wrapper starting with '('.");
    consume(parser, TOKEN_RPARETH, "Expected closing parameter list wrapper ')'.");
    

    //verifies the return type...
    consume(parser, TOKEN_ARROW, "Expected return signature pointer token '->'.");
    consume(parser, TOKEN_LPARETH, "Expected open parenthesis '(' around return type specification.");
    consume(parser, TOKEN_VOID, "Expected explicit type parameter keyword 'void'.");
    consume(parser, TOKEN_RPARETH, "Expected closing parenthesis ')' around return type specification.");

    //if successful parse the block.
    parse_block(parser);
}


void compile_parse(Lexer* lexer){
    Parser parser;
    parser.lexer = lexer;

    //Prime  by fetching the first token. That way parser.current is not NULL
    advance(&parser);

    //Now parsing the function.
    parse_function(&parser);

    //Checks the end of the file.
    consume(&parser, TOKEN_EOF, "Unexpected trailing syntax tokens encountered after main entry block.");

}