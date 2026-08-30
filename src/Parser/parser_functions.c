#include "../../headers/parser.h"
#include <stdbool.h>
#include <stdint.h>

ASTNode *function(Parser *parser, CodegenContext *context) {
  advance(parser);
  int pointer_level = 0;
  MemoryBucket b = BUCKET_PLUS_ONE;
  consume(parser, TOKEN_LPARETH,
          "Expected parameter list wrapper starting with '('.");

  if (parser->current.type != TOKEN_RPARETH) {
    parser->lexer->fxn->params = get_params(context, parser);
  }
  consume(parser, TOKEN_RPARETH,
          "Expected closing parameter list wrapper ')'.");

  if (parser->current.type == TOKEN_ARROW) {
    advance(parser);
    if (parser->current.type != TOKEN_LBRACE) {
      if(parser->current.type == TOKEN_SIGIL){
          parser->lexer->fxn->bucket = BUCKET_THREE;
        advance(parser);
      }
      TokenType ReturnType = parser->current.type;
      bool ret_arr = false;
      advance(parser);
      while(parser->current.type == TOKEN_MUL){
        pointer_level++;
        advance(parser);
      }

      if(parser->current.type == TOKEN_LSQUARE_BRACE){
        ret_arr = true;
        advance(parser);
        if(parser->current.type != TOKEN_RSQUARE_BRACE){
          parser->lexer->fxn->bucket = BUCKET_TWO;
          uint32_t len = token_to_int(parser->current);
          parser->lexer->fxn->array_count = len;
          advance(parser);
        }

        consume(parser, TOKEN_RSQUARE_BRACE, "Expected ']'");
      }

      switch (ReturnType) {
      case DECLARE_INT:
        parser->lexer->fxn->return_type = (ret_arr) ? TYPE_INT_ARRAY : TYPE_INT;
        break;
      case DECLARE_STR:
        parser->lexer->fxn->return_type = TYPE_STRING;
        break;
      case DECLARE_FLOAT:
        parser->lexer->fxn->return_type = (ret_arr) ? TYPE_FLOAT_ARRAY : TYPE_FLOAT;
        break;
      case DECLARE_BOOL:
        parser->lexer->fxn->return_type = (ret_arr) ? TYPE_BOOL_ARRAY : TYPE_BOOL;
        break;
      case TOKEN_VOID:
        parser->lexer->fxn->return_type = TYPE_NULL;
        break;
      case DECLARE_CHAR:
        parser->lexer->fxn->return_type = (ret_arr) ? TYPE_CHAR_ARRAY : TYPE_CHAR;
        break;
      default:
        error(parser, "Invalid return type", SYNTAXERROR);
      }


    }
  }
  //advance(parser);
  ASTNode *body = parse_block(parser, context);

  int name_length = strlen(parser->lexer->fxn->name);
  return create_function_node(parser->lexer->fxn->name, name_length, body,
                              *parser->lexer->fxn, parser->lexer->scope_level, context->a, pointer_level);
}


ASTNode *parse_function(Parser *parser, CodegenContext *context) {
  advance(parser);

  switch (parser->current.type) {
  case TOKEN_RUN:
  case TOKEN_IDENTIFIER: {
    return register_and_form_fxn(parser, context);
  }
  default:
    error(parser, "Cannot use token to create fxn.", SYNTAXERROR);
    advance(parser);
    return NULL;
  }
}

ASTNode *parse_fxn_call(Parser *parser, CodegenContext *context,
                               char *name, int name_length) {
  consume(parser, TOKEN_LPARETH, "Expected '(' after function name.");
  Args *args;
  if (parser->current.type != TOKEN_RPARETH) {
    _DEBUG("PARAMETERS.")
    args = get_args(context, parser);
  } else {
    _DEBUG("NO PARAMETERS.")
    args = NULL;
  }

  
  consume(parser, TOKEN_RPARETH, "Expected ')' after arguments.");
  _DEBUG("Done allocating.")
  return create_fxn_call_node(name, name_length,*parser->lexer->fxn,
                              parser->lexer->scope_level, args ? args : NULL, context->a);
}
