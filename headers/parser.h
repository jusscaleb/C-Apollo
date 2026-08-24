/*===================================================================
                          parser.h

                        (c)2026 SCXRPIUS.dev

              The Apollo Parser Compile time library.
          Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#ifndef PARSER_H
#define PARSER_H

#include "../headers/ast.h"
#include "../headers/defs.h"
#include "../headers/error.h"
#include "../headers/functions.h"
#include "../headers/memory.h"
#include "../headers/token.h"
#include "../headers/variables.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

ASTNode *parse_logical_and(Parser *parser, CodegenContext *context);

ASTNode *parse_logical_or(Parser *parser, CodegenContext *context);
ASTNode *parse_body_statement(Parser *parser, CodegenContext *context);
ASTNode *parse_block(Parser *parser, CodegenContext *context);
ASTNode *parse_function(Parser *parser, CodegenContext *context);
Params *get_params(CodegenContext *context, Parser *parser);
Args *get_args(CodegenContext *context, Parser *parser);
ASTNode *parse_fxn_call(Parser *parser, CodegenContext *context, char *name,
                        int name_length);
ASTNode *var(Parser *parser, CodegenContext *context, DataType var_type);
void advance(Parser *parser);
void synchronize(Parser *parser, TokenType safe_token);
void synchronize(Parser *parser, TokenType safe_token);
ASTNode *parse_block(Parser *parser, CodegenContext *context);

void consume(Parser *parser, TokenType type, const char *errorMessage);

ASTNode *register_and_form_fxn(Parser *parser, CodegenContext *context);

ASTNode *function(Parser *parser, CodegenContext *context);

Token parser_get_var_token(Parser *parser, DataType dt);

ASTNode *parse_urinary(Parser *parser, CodegenContext *context);

#endif