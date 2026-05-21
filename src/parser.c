/*Structure Builder */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../headers/token.h"
#include "../headers/arithmetic.h"
#include "../headers/variables.h"




//move on to next token
static void advance(Parser* parser){
    parser->previous = parser->current;
    parser->current = next_token(parser->lexer);
}



//checks if given token matches expected token.
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

/*------------------ARITHMETICS------------------*/
static ExprResult parse_expression(Parser* parser, CodegenContext* context);

static ExprResult parse_primary(Parser* parser, CodegenContext* context){
    Token token = parser->current;

    if(token.type == TOKEN_INT){
        consume(parser, TOKEN_INT, "Expected integer literal.");
        return make_literal_expr(token);
    }

    if(token.type == TOKEN_FLOAT){
        consume(parser, TOKEN_FLOAT, "Expected float literal.");
        return make_literal_expr(token);
    }

    fprintf(stderr, "Apollo Syntax Error [Line %d]: Expected expression value.\n", parser->current.line);
    exit(1);
}

static ExprResult parse_factor(Parser* parser, CodegenContext* context){
    ExprResult left = parse_primary(parser, context);

    while(
        parser->current.type == TOKEN_MUL ||
        parser->current.type == TOKEN_DIV ||
        parser->current.type == TOKEN_MOD
    ){
        TokenType operator_type = parser->current.type;
        advance(parser);

        ExprResult right = parse_primary(parser, context);
        left = gen_binary_expr(context, left, operator_type, right);
    }

    return left;
}

static ExprResult parse_expression(Parser* parser, CodegenContext* context){
    ExprResult left = parse_factor(parser, context);

    while(
        parser->current.type == TOKEN_ADD ||
        parser->current.type == TOKEN_SUB
    ){
        TokenType operator_type = parser->current.type;
        advance(parser);

        ExprResult right = parse_factor(parser, context);
        left = gen_binary_expr(context, left, operator_type, right);
    }

    return left;
}


static void println(Parser* parser, CodegenContext* context){
        advance(parser);
        consume(parser, TOKEN_LPARETH, "Expected open parenthesis '(' for arguments.");

    // Keep a copy of the string token before consume() advances past it.
    Token string_token = parser->current;

    switch(parser->current.type){
        case TOKEN_STRING:{
    consume(parser, TOKEN_STRING, "Expected string literal argument inside function call.");
    // Send the string literal over to the backend while its token data is available.
    gen_println_statement(context, string_token.start, string_token.length);
    break;
    }
    case TOKEN_INT:
    case TOKEN_FLOAT:{
            ExprResult result = parse_expression(parser, context);
            gen_println_expr(context, result);
            break;
        }
   
    default:
       printf("");
       break;
}
    consume(parser, TOKEN_RPARETH, "Expected closing parenthesis ')' for arguments.");
    consume(parser, TOKEN_SEMICOLON, "Expected trailing semicolon ';' to terminate statement.");
}

void var(Parser* parser, CodegenContext* context){
    advance(parser);
    consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");
    
    
    Variable variable = {TYPE_INT, parser->current.start, parser->current.length, parser->current.line};

    switch (parser->current.type) {
        case TOKEN_INT:
        {
            consume(parser, TOKEN_INT, "Expected integer literal for variable assignment.");
            create_var(context, variable.start, variable.length);
            break;
            
        }

        default:{
            fprintf(stderr, "Apollo Syntax error [Line %d], expected variable value", parser->current.line);
            exit(1);
        }
    
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
    
}

//check the body statement
static void parse_body_statement(Parser* parser, CodegenContext* context) {
    //consume(parser, TOKEN_IDENTIFIER, "Expected function identifier statement inside block.");
    if(parser->current.type == TOKEN_PRINTLN)println(parser, context);
    else if(parser->current.type == TOKEN_VAR)var(parser, context);
    

}

//parse_block() checks the innard of that function.
static void parse_block(Parser* parser, CodegenContext* context){
    consume(parser, TOKEN_LBRACE, "Expected open brace '{' to begin function block definition.");

    while(parser->current.type != TOKEN_RBRACE && parser->current.type != TOKEN_EOF){
        parse_body_statement(parser, context);

    }

    consume(parser, TOKEN_RBRACE, "Expected closing brace '}' to terminate block context.");
}

//parse_function check if the function is grammatically correct. (fxn run()->(void){})
void parse_function(Parser* parser, CodegenContext* context){
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

    // The full function signature is valid, so the backend can open the LLVM function.
    gen_function_start(context, "run");

    // Parse each statement in the body and emit its matching backend code.
    parse_block(parser, context);

    // Close the generated main function after the source block has been parsed.
    gen_function_end(context, true);
}


void compile_parse(Lexer* lexer){
    Parser parser;
    parser.lexer = lexer;
    //Prime  by fetching the first token. That way parser.current is not NULL
    advance(&parser);

    //Startup the backend
    CodegenContext code_writer;
    codegen_init(&code_writer, "output.ll"); 

    //Now parsing the function while sharing the backend context.
    parse_function(&parser, &code_writer);

    //Checks the end of the file.
    consume(&parser, TOKEN_EOF, "Unexpected trailing syntax tokens encountered after main entry block.");

    printf("SUCCESSFUL.\n");

}
