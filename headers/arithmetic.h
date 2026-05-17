#ifndef ARITHMETIC_H
#define ARITHMETIC_H


#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "token.h"

static void arithmetic_advance(Parser* parser){
    parser->previous = parser->current;
    parser->current = next_token(parser->lexer);

}

static void arithmetic_consume(Parser* parser, TokenType type, const char* errorMessage){
    if(parser->current.type == type){
        arithmetic_advance(parser);
        return;
    }

    fprintf(stderr, "Apollo Syntax Error [Line %d]: %s\n", parser->current.line, errorMessage);

    exit(EXIT_FAILURE);
}

static void parse_addition(Parser*parser, CodegenContext* context, Token left){
    arithmetic_consume(parser, TOKEN_ADD, "Expected '+'.");

    Token right = parser->current;

    if(left.type == TOKEN_INT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected '+'.");

        gen_println_integer_addition(context, 
            left.start,
            left.length,
            right.start,
            right.length);

        return;
    }
    if (left.type == TOKEN_FLOAT && right.type == TOKEN_FLOAT) {
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected float after '+'.");
        gen_println_float_addition(
            context,
            left.start,
            left.length,
            right.start,
            right.length
        );
        return;
    }

    if (left.type == TOKEN_INT && right.type == TOKEN_FLOAT) {
    arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '+'.");

    gen_println_mixed_addition(
        context,
        left.start,
        left.length,
        right.start,
        right.length,
        true
    );

    return;
}

if (left.type == TOKEN_FLOAT && right.type == TOKEN_INT) {
    arithmetic_consume(parser, TOKEN_INT, "Expected int after '+'.");

    gen_println_mixed_addition(
        context,
        left.start,
        left.length,
        right.start,
        right.length,
        false
    );

    return;
}
    fprintf(stderr,
            "Apollo Syntax Error [Line %d]: Cannot add different literal types yet.\n",
            parser->current.line);
    exit(1);
}

static void parse_multiplication(Parser* parser, CodegenContext* context, Token left){
    arithmetic_consume(parser, TOKEN_MUL, "Expected '*'.");

    Token right = parser->current;

    if(left.type == TOKEN_INT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '*'.");

        gen_println_integer_multiplication(
            context,
            left.start,
            left.length,
            right.start,
            right.length
        );

        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '*'.");

        gen_println_float_multiplication(
            context,
            left.start,
            left.length,
            right.start,
            right.length
        );

        return;
    }

    if(left.type == TOKEN_INT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '*'.");

        gen_println_mixed_multiplication(
            context,
            left.start,
            left.length,
            right.start,
            right.length,
            true
        );

        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '*'.");

        gen_println_mixed_multiplication(
            context,
            left.start,
            left.length,
            right.start,
            right.length,
            false
        );

        return;
    }

    fprintf(stderr,
            "Apollo Syntax Error [Line %d]: Cannot multiply these literal types yet.\n",
            parser->current.line);
    exit(1);
}

static void parse_subtraction(Parser* parser, CodegenContext* context, Token left){
    arithmetic_consume(parser, TOKEN_SUB, "Expected '-'.");

    Token right = parser->current;

    if(left.type == TOKEN_INT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '-'.");
        gen_println_integer_subtraction(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '-'.");
        gen_println_float_subtraction(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_INT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '-'.");
        gen_println_mixed_subtraction(context, left.start, left.length, right.start, right.length, true);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '-'.");
        gen_println_mixed_subtraction(context, left.start, left.length, right.start, right.length, false);
        return;
    }

    fprintf(stderr,
            "Apollo Syntax Error [Line %d]: Cannot subtract these literal types yet.\n",
            parser->current.line);
    exit(1);
}

static void parse_division(Parser* parser, CodegenContext* context, Token left){
    arithmetic_consume(parser, TOKEN_DIV, "Expected '/'.");

    Token right = parser->current;

    if(left.type == TOKEN_INT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '/'.");
        gen_println_integer_division(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '/'.");
        gen_println_float_division(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_INT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '/'.");
        gen_println_mixed_division(context, left.start, left.length, right.start, right.length, true);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '/'.");
        gen_println_mixed_division(context, left.start, left.length, right.start, right.length, false);
        return;
    }

    fprintf(stderr,
            "Apollo Syntax Error [Line %d]: Cannot divide these literal types yet.\n",
            parser->current.line);
    exit(1);
}

static void parse_modulus(Parser* parser, CodegenContext* context, Token left){
    arithmetic_consume(parser, TOKEN_MOD, "Expected '%'.");

    Token right = parser->current;

    if(left.type == TOKEN_INT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '%'.");
        gen_println_integer_modulus(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '%'.");
        gen_println_float_modulus(context, left.start, left.length, right.start, right.length);
        return;
    }

    if(left.type == TOKEN_INT && right.type == TOKEN_FLOAT){
        arithmetic_consume(parser, TOKEN_FLOAT, "Expected decimal after '%'.");
        gen_println_mixed_modulus(context, left.start, left.length, right.start, right.length, true);
        return;
    }

    if(left.type == TOKEN_FLOAT && right.type == TOKEN_INT){
        arithmetic_consume(parser, TOKEN_INT, "Expected int after '%'.");
        gen_println_mixed_modulus(context, left.start, left.length, right.start, right.length, false);
        return;
    }

    fprintf(stderr,
            "Apollo Syntax Error [Line %d]: Cannot use modulus with these literal types yet.\n",
            parser->current.line);
    exit(1);
}

#endif
