#include "../../headers/parser.h"

// 1st Priority
ASTNode *parse_primary(Parser *parser, CodegenContext *context) {
  Token token = parser->current;

  switch (token.type) {
  case TOKEN_INT: {
    consume(parser, TOKEN_INT, "Expected integer literal.");
    return create_literal_node(token, context->a);
  }
  case TOKEN_STRING: {
    consume(parser, TOKEN_STRING, "Expected string literal.");
    return create_literal_node(token, context->a);
  }
  case TOKEN_BOOL:
  case TOKEN_NULL: {
    (token.type == TOKEN_BOOL)
        ? consume(parser, TOKEN_BOOL, "Expected boolean literal.")
        : consume(parser, TOKEN_NULL, "Expected null value literal.");
    return create_literal_node(token, context->a);
  }

  case TOKEN_FLOAT: {
    consume(parser, TOKEN_FLOAT, "Expected float literal.");
    return create_literal_node(token, context->a);
  }
  case TOKEN_LPARETH:{
    consume(parser, TOKEN_LPARETH, "");
    ASTNode *expr = parse_logical_or(parser,context);
    consume(parser, TOKEN_RPARETH, "Expected ')' to close grouped expression.");
    return expr;
  }


  case TOKEN_IDENTIFIER: {
    advance(parser);
    if (parser->current.type == TOKEN_LPARETH) {
      char *potential_name = (char *)arena_alloc(context->a, token.length + 1);
      memcpy(potential_name, token.start, token.length);
      potential_name[token.length] = '\0';
      ASTNode *call_node =
          parse_fxn_call(parser, context, potential_name, token.length);
      return call_node;
    }
    return create_var_ref_node(token.start, token.length, *parser->lexer->fxn,
                               parser->lexer->scope_level, context->a);
  }
  case TOKEN_COMMA: {
    advance(parser);
    return parse_primary(parser, context);
  }
  case TOKEN_CHAR: {
    consume(parser, TOKEN_CHAR, "Expected Character Literal");
    return create_literal_node(token, context->a);
  }
  }
  if (!(parser->current.type == TOKEN_EOF && parser->lexer->errors->size > 0)) {
    error(parser, "Expected expression value.", SYNTAXERROR);
  }
  advance(parser);
  return NULL;
}

// Second Priority
ASTNode *parse_factor(Parser *parser, CodegenContext *context) {
  ASTNode *left = parse_primary(parser, context);

  while (parser->current.type == TOKEN_MUL ||
         parser->current.type == TOKEN_DIV ||
         parser->current.type == TOKEN_MOD) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_primary(parser, context);
    left = create_binary_node(left, operator_type, right, context->a);
  }

  return left;
}

// Third Priority
ASTNode *parse_expression(Parser *parser, CodegenContext *context) {
  ASTNode *left = parse_factor(parser, context);

  while (parser->current.type == TOKEN_ADD ||
         parser->current.type == TOKEN_SUB) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_factor(parser, context);
    left = create_binary_node(left, operator_type, right, context->a);
  }

  return left;
}

// Fourth Priority
ASTNode *parse_comparison(Parser *parser, CodegenContext *context) {
  ASTNode *left = parse_expression(parser, context);

  while (parser->current.type == TOKEN_GT || parser->current.type == TOKEN_ST ||
         parser->current.type == TOKEN_GE || parser->current.type == TOKEN_SE ||
         parser->current.type == TOKEN_EQT ||
         parser->current.type == TOKEN_NEQ) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_expression(parser, context);
    left = create_binary_node(left, operator_type, right, context->a);
  }

  return left;
}

// Fifth Priority
ASTNode *parse_logical_and(Parser *parser, CodegenContext *context) {
  ASTNode *left = parse_comparison(parser, context);
  while (parser->current.type == TOKEN_AND) {
    TokenType operator_type = parser->current.type;
    advance(parser);
    ASTNode *right = parse_comparison(parser, context);
    left = create_binary_node(left, operator_type, right, context->a);
  }
  return left;
}

// High priority
ASTNode *parse_logical_or(Parser *parser, CodegenContext *context) {
  ASTNode *left = parse_logical_and(parser, context);
  while (parser->current.type == TOKEN_OR) {
    TokenType operator_type = parser->current.type;
    advance(parser);
    ASTNode *right = parse_logical_and(parser, context);
    left = create_binary_node(left, operator_type, right, context->a);
  }
  return left;
}

ASTNode *parse_block(Parser *parser, CodegenContext *context) {
  consume(parser, TOKEN_LBRACE, "Expected open brace '{' to begin block.");

  ASTNode *block = create_block_node(parser->lexer->fxn->level, context->a);
  parser->lexer->fxn->block_nodes++;

  while (parser->current.type != TOKEN_RBRACE &&
         parser->current.type != TOKEN_EOF) {
    ASTNode *stmt = parse_body_statement(parser, context);
    if (stmt != NULL) {
      block_add_statement(block, stmt, context->a);
    }
  }

  consume(parser, TOKEN_RBRACE,
          "Expected closing brace '}' to terminate block.");

  return block;
}
