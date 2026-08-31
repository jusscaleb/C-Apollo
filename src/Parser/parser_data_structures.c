#include "../../headers/parser.h"
#include <stdint.h>
#include <string.h>

ASTNode *parse_array_node(Parser *parser, DataType datatype,
                          CodegenContext *context, int count) {
  consume(parser, TOKEN_LBRACE, "Expected '{' to open array.");

  bool is_dynamic = (count <= 0);
  uint32_t capacity = (count > 0) ? (uint32_t)count : 8;
  uint32_t parsed_count = 0;
  ASTNode **elements =
      (ASTNode **)arena_alloc(context->a, sizeof(ASTNode *) * capacity);

  while (parser->current.type != TOKEN_RBRACE &&
         parser->current.type != TOKEN_EOF) {
    if (parsed_count >= capacity) {
      uint32_t old_capacity = capacity;
      capacity *= 2;
      ASTNode **new_elements =
          (ASTNode **)arena_alloc(context->a, sizeof(ASTNode *) * capacity);
      memcpy(new_elements, elements, sizeof(ASTNode *) * old_capacity);
      elements = new_elements;
    }

    ASTNode *elem = parse_logical_or(parser, context);
    elements[parsed_count++] = elem;

    if (parser->current.type == TOKEN_COMMA) {
      advance(parser);
    } else {
      break;
    }
  }

  consume(parser, TOKEN_RBRACE, "Expected '}' to close array.");

  uint32_t final_capacity = (count > 0) ? (uint32_t)count : parsed_count;
  return create_array_node(elements, parsed_count, final_capacity, is_dynamic,
                           datatype, context->a);
}

ASTNode *parse_index_expr(Parser *parser, CodegenContext *context,
                          ASTNode *target) {
  consume(parser, TOKEN_LSQUARE_BRACE, "Expected '[' for array indexing.");
  ASTNode *index = parse_logical_or(parser, context);
  consume(parser, TOKEN_RSQUARE_BRACE,
          "Expected ']' after array index expression.");

  return create_index_expr_node(target, index, TYPE_NULL, context->a);
}


ASTNode *parse_struct_type(Parser *parser, CodegenContext *context) {
  advance(parser);

  if (parser->current.type != TOKEN_IDENTIFIER) {
    error(parser, "Expected name to define struct.", SYNTAXERROR);
  }

  int name_length = parser->current.length;
  char *name = (char *)arena_alloc(context->a, name_length + 1);
  memcpy(name, parser->current.start, name_length);
  name[name_length] = '\0';

  advance(parser);
  consume(parser, TOKEN_LBRACE, "Expected '{'.");

  Struct *fields = parse_struct_fields(parser, context);

  consume(parser, TOKEN_RBRACE, "Expected '}'.");
  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end struct definition.");

  return create_struct_node(fields, parser->lexer->fxn, parser->lexer->scope_level, context->a, name, name_length);
}

Struct *parse_struct_fields(Parser *parser, CodegenContext *context) {
  if (parser->current.type == TOKEN_RBRACE || parser->current.type == TOKEN_EOF) {
    return NULL;
  }

  Struct *f = (Struct *)arena_alloc(context->a, sizeof(Struct));
  f->field = NULL;
  f->next = NULL;
  MemoryBucket b = BUCKET_PLUS_ONE;

  if(parser->current.type == TOKEN_SIGIL){
    error(parser, "Not allowed.", SYNTAXERROR);
  }
  switch (parser->current.type) {
  case DECLARE_INT:
    f->field = var(parser, context, TYPE_INT, b);
    break;

  case DECLARE_FLOAT:
    f->field = var(parser, context, TYPE_FLOAT, b);
    break;

  case DECLARE_STR:
    f->field = var(parser, context, TYPE_STRING, b);
    break;

  case DECLARE_BOOL:
    f->field = var(parser, context, TYPE_BOOL, b);
    break;

  case DECLARE_CHAR:
    f->field = var(parser, context, TYPE_CHAR, b);
    break;

  case TOKEN_IDENTIFIER: {
    const int TYPE_LEN = parser->current.length;
    char *type_name = (char *)arena_alloc(context->a, TYPE_LEN + 1);
    memcpy(type_name, parser->current.start, TYPE_LEN);
    type_name[TYPE_LEN] = '\0';
    advance(parser);

    int ptr_level = 0;
    while (parser->current.type == TOKEN_MUL) {
      ptr_level++;
      advance(parser);
    }
    Token field_name_token = parser->current;
    consume(parser, TOKEN_IDENTIFIER, "Expected field identifier.");

    ASTNode *var_node = create_var_decl_node(field_name_token.start, field_name_token.length,
                                             TYPE_STRUCT, NULL, *parser->lexer->fxn,
                                             parser->lexer->scope_level, context->a,
                                             ptr_level, b, -1);
    var_node->var_decl.struct_type_name = type_name;
    var_node->var_decl.struct_type_name_len = TYPE_LEN;
    f->field = var_node;
    break;
  }

  default:
    error(parser, "Expected field datatype inside struct.", SYNTAXERROR);
    return NULL;
}
  advance(parser);
  f->next = parse_struct_fields(parser, context);
  return f;
}

ASTNode *parse_fxns_block(Parser *parser, CodegenContext *context) {
  advance(parser); // Advance past 'fxns'

  if (parser->current.type != TOKEN_IDENTIFIER) {
    error(parser, "Expected identifier for namespace/struct after 'fxns'.", SYNTAXERROR);
  }

  int name_length = parser->current.length;
  char *name = (char *)arena_alloc(context->a, name_length + 1);
  memcpy(name, parser->current.start, name_length);
  name[name_length] = '\0';

  advance(parser);
  consume(parser, TOKEN_LBRACE, "Expected '{' to open 'fxns' block.");

  Fxns *fxns = (Fxns *)arena_alloc(context->a, sizeof(Fxns));
  fxns->length = name_length;
  fxns->level = parser->lexer->scope_level;
  fxns->parent_fxn = parser->lexer->fxn;
  fxns->parent = NULL;
  fxns->bucket = BUCKET_ONE;

  ASTNode *fxns_node = create_fxns_node(name, name_length, parser->lexer->scope_level, fxns, context->a);

  while (parser->current.type != TOKEN_RBRACE && parser->current.type != TOKEN_EOF) {
    ASTNode *method = NULL;
    if (parser->current.type == TOKEN_FXN) {
      advance(parser);
      if (parser->current.type == TOKEN_IDENTIFIER) {
        method = register_and_form_method(parser, context, name, name_length);
      } else {
        error(parser, "Expected identifier after 'fxn' in 'fxns' block.", SYNTAXERROR);
      }
    } else if (parser->current.type == TOKEN_IDENTIFIER) {
      method = register_and_form_method(parser, context, name, name_length);
    } else {
      error(parser, "Expected method definition inside 'fxns' block.", SYNTAXERROR);
      advance(parser);
    }

    if (method) {
      fxns_add_method(fxns_node, method, context->a);
    }
  }

  consume(parser, TOKEN_RBRACE, "Expected '}' to close 'fxns' block.");
  return fxns_node;
}