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
                              .block_nodes = 0,
                              .array_count = -1};
  ASTNode *program_block = create_block_node(parser->lexer->fxn->level, context->a);

  while (parser->current.type == DECLARE_STRUCT || parser->current.type == TOKEN_VAR ||
         parser->current.type == DECLARE_INT || parser->current.type == DECLARE_STR ||
         parser->current.type == DECLARE_FLOAT || parser->current.type == DECLARE_BOOL ||
         parser->current.type == DECLARE_CHAR ||
         parser->current.type == TOKEN_FXN || parser->current.type == TOKEN_FXNS) {
    if (parser->current.type == DECLARE_STRUCT) {
      ASTNode *struct_node = parse_struct_type(parser, context);
      if (struct_node) {
        block_add_statement(program_block, struct_node, context->a);
      }
    } else if (parser->current.type == TOKEN_FXNS) {
      ASTNode *fxns_node = parse_fxns_block(parser, context);
      if (fxns_node) {
        block_add_statement(program_block, fxns_node, context->a);
      }
    } else if (parser->current.type == TOKEN_VAR || parser->current.type == DECLARE_INT ||
               parser->current.type == DECLARE_STR || parser->current.type == DECLARE_FLOAT ||
               parser->current.type == DECLARE_BOOL || parser->current.type == DECLARE_CHAR) {
      DataType dt = TYPE_NULL;
      switch (parser->current.type) {
      case DECLARE_BOOL: dt = TYPE_BOOL; break;
      case DECLARE_FLOAT: dt = TYPE_FLOAT; break;
      case DECLARE_STR: dt = TYPE_STRING; break;
      case DECLARE_INT: dt = TYPE_INT; break;
      case DECLARE_CHAR: dt = TYPE_CHAR; break;
      default: break;
      }
      MemoryBucket b = (parser->previous.type == TOKEN_SIGIL) ? BUCKET_THREE : BUCKET_ONE;
      ASTNode *var_node = var(parser, context, dt, b);
      consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
      if (var_node) {
        block_add_statement(program_block, var_node, context->a);
      }
    } else if (parser->current.type == TOKEN_FXN) {
      ASTNode *fxn_node = parse_function(parser, context);
      if (fxn_node) {
        block_add_statement(program_block, fxn_node, context->a);
      }
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
