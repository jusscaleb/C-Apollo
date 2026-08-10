#include "../../headers/parser.h"


// move on to next token
__attribute__((always_inline)) void advance(Parser *parser) {
  parser->previous = parser->current;
  parser->current = next_token(parser->lexer);
}


void change_active_state(CodegenContext *context, Lexer *lexer) {
  int new_level = lexer->scope_level;
  // printf("Symbol Count: %s\n", context->symbols[0].name);
  ASTNode var;
  for (int i = context->symbol_count - 1; i >= 0; i--) {
    if (context->symbols[i].scope_level > new_level) {
      context->symbols[i].is_active = false;
    }
  }
}

void synchronize(Parser *parser, TokenType safe_token) {
  advance(parser);
  while (parser->current.type != TOKEN_EOF &&
         parser->current.type != safe_token)
    advance(parser);
}

__attribute__((always_inline)) void consume(Parser *parser, TokenType type, const char *errorMessage) {
  if (parser->current.type == type) {
    advance(parser);
    return;
  }
  // Prevent cascaded errors at EOF if we already reported an error
  if (parser->current.type == TOKEN_EOF && parser->lexer->errors->size > 0) {
    return;
  }
  error(parser, errorMessage, SYNTAXERROR);
  synchronize(parser, TOKEN_SEMICOLON);
}

ASTNode *register_and_form_fxn(Parser *parser, CodegenContext *context) {

  Token name_token = parser->current;

  const int NAME_LENGTH = name_token.length;
  char *name = (char *)arena_alloc(context->a, NAME_LENGTH+1);
  memcpy(name, name_token.start, NAME_LENGTH);
  name[NAME_LENGTH] = '\0';

  Fxn *parent = parser->lexer->fxn;
  Fxn *fxn = (Fxn *)arena_alloc(context->a, sizeof(Fxn));

  fxn->length = NAME_LENGTH;
  fxn->name = name;
  fxn->level = parser->lexer->scope_level;
  fxn->line = parser->lexer->line;
  fxn->parent_fxn = parent;
  fxn->return_type = TYPE_NULL;

  parser->lexer->fxn = fxn;

  ASTNode *node = function(parser, context);

  parser->lexer->fxn = parent;
  return node;
}

Params *get_params(CodegenContext *context, Parser *parser) {
  Params *p = (Params *)arena_alloc(context->a, sizeof(Params));

  switch (parser->current.type) {
  case DECLARE_INT: {
    p->param = var(parser, context, TYPE_INT);
    break;
  }

  case DECLARE_FLOAT: {
    p->param = var(parser, context, TYPE_FLOAT);
    break;
  }

  case DECLARE_STR: {
    p->param = var(parser, context, TYPE_STRING);
    break;
  }

  case DECLARE_BOOL: {
    p->param = var(parser, context, TYPE_BOOL);
    break;
  }

  default: {
    return NULL;
  }
  }
  if (parser->current.type == TOKEN_COMMA) {
    advance(parser);
    p->next = get_params(context, parser);
  } else {
    p->next = NULL; // base case
  }

  return p;
}

Args *get_args(CodegenContext *context, Parser *parser) {
  _DEBUG("Function calling");
  Args *a = (Args *)arena_alloc(context->a, sizeof(Args));

  a->arg = parse_logical_or(parser, context);
  if (parser->current.type == TOKEN_COMMA) {
    advance(parser);
    a->next = get_args(context, parser);
  } else {
    a->next = NULL;
  }

  return a;
}


__attribute__((always_inline))Token parser_get_var_token(Parser *parser, DataType dt){
    switch (dt) {
    case TYPE_INT:    return (Token){TOKEN_INT, "0", 1, parser->current.line};
    case TYPE_STRING: return (Token){TOKEN_STRING, "null", 4, parser->current.line};
    case TYPE_FLOAT:  return (Token){TOKEN_FLOAT, "0.0", 3, parser->current.line};
    case TYPE_BOOL:   return (Token){TOKEN_BOOL, "null", 4, parser->current.line};
    case TYPE_NULL:   return(Token){TOKEN_NULL, "null", 4, parser->current.line};
    case TYPE_CHAR:   return (Token){TOKEN_CHAR, "", 0, parser->current.line};
  }
}

