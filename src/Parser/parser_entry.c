#include "../../headers/parser.h"



ASTNode *begin(Parser *parser, CodegenContext *context) {
  parser->lexer->fxn = arena_alloc(context->a, sizeof(Fxn));
  *parser->lexer->fxn = (Fxn){.length = 0,
                              .level = 0,
                              .line = 0,
                              .return_type = TYPE_NULL,
                              .name = "global",
                              .parent_fxn = NULL,
                              .ret_nodes = 0,
                              .block_nodes = 0};
  ASTNode *program_block = create_block_node(parser->lexer->fxn->level, context->a);

  // Checking global variables
  while (parser->current.type == TOKEN_VAR) {
    ASTNode *var_node = var(parser, context, TYPE_NULL);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
    if (var_node) {
      block_add_statement(program_block, var_node, context->a);
    }
  }

  while (parser->current.type == TOKEN_FXN) {
    ASTNode *fxn_node = parse_function(parser, context);
    if (fxn_node) {
      block_add_statement(program_block, fxn_node, context->a);
    }
  }

  return create_program_node(program_block, context->a);
}

ASTNode *compile_parse(Lexer *lexer, ErrorStack *s, CodegenContext *context) {
  Parser parser = {0};
  parser.lexer = lexer;

  parser.error = s;
  // Prime by fetching the first token. That way parser.current is not NULL
  _DEBUG("[PARSER] Priming first token.")
  fflush(stderr);
  advance(&parser);

  // Now parsing the function while sharing the symbol context.
  _DEBUG("[PARSER] Entering begin().")
  fflush(stderr);
  ASTNode *program_node = begin(&parser, context);
  _DEBUG("[PARSER] begin() finished.")
  fflush(stderr);

  // Checks the end of the file.
  consume(
      &parser, TOKEN_EOF,
      "Unexpected trailing syntax tokens encountered after main entry block.");
  _DEBUG("[PARSER] EOF consumed.");
  fflush(stderr);
  return program_node;
}
