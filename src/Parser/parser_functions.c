#include "../../headers/parser.h"




ASTNode *function(Parser *parser, CodegenContext *context) {
  advance(parser);
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
      switch (parser->current.type) {
      case DECLARE_INT:
        parser->lexer->fxn->return_type = TYPE_INT;
        break;
      case DECLARE_STR:
        parser->lexer->fxn->return_type = TYPE_STRING;
        break;
      case DECLARE_FLOAT:
        parser->lexer->fxn->return_type = TYPE_FLOAT;
        break;
      case DECLARE_BOOL:
        parser->lexer->fxn->return_type = TYPE_BOOL;
        break;
      case TOKEN_VOID:
        parser->lexer->fxn->return_type = TYPE_NULL;
        break;
      default:
        error(parser, "Invalid return type", SYNTAXERROR);
      }
      advance(parser);
    }
  }

  ASTNode *body = parse_block(parser, context);

  int name_length = strlen(parser->lexer->fxn->name);
  return create_function_node(parser->lexer->fxn->name, name_length, body,
                              *parser->lexer->fxn, parser->lexer->scope_level, context->a);
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
