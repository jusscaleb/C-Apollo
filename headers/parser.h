#ifndef PARSER_H
#define PARSER_H

#include "../headers/ast.h"
#include "../headers/defs.h"
#include "../headers/error.h"
#include "../headers/functions.h"
#include "../headers/token.h"
#include "../headers/variables.h"
#include "../headers/memory.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static ASTNode *parse_logical_and(Parser *parser, CodegenContext *context);

static ASTNode *parse_logical_or(Parser *parser, CodegenContext *context);
static ASTNode *parse_body_statement(Parser *parser, CodegenContext *context);
static ASTNode *parse_block(Parser *parser, CodegenContext *context);
ASTNode *parse_function(Parser *parser, CodegenContext *context);
void change_active_state(CodegenContext *context, Lexer *lexer);
Params *get_params(CodegenContext *context, Parser *parser);
Args *get_args(CodegenContext *context, Parser *parser);
static ASTNode *parse_fxn_call(Parser *parser, CodegenContext *context,
                               char *name, int name_length);


#endif