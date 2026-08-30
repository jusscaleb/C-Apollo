#include "../../headers/parser.h"


// move on to next token
__attribute__((always_inline)) void advance(Parser *parser) {
  parser->previous = parser->current;
  parser->current = next_token(parser->lexer);
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

ASTNode *register_and_form_method(Parser *parser, CodegenContext *context, const char *prefix, int prefix_len) {
  Token name_token = parser->current;

  const int NAME_LENGTH = prefix_len + 1 + name_token.length;
  char *name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);
  snprintf(name, NAME_LENGTH + 1, "%.*s_%.*s", prefix_len, prefix, name_token.length, name_token.start);

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
  MemoryBucket b = 0;

  if(parser->current.type == TOKEN_SIGIL){
    b = BUCKET_THREE;
    advance(parser);
  }

  switch (parser->current.type) {
  case DECLARE_INT: {
    p->param = var(parser, context, TYPE_INT, b);
    break;
  }

  case DECLARE_FLOAT: {
    p->param = var(parser, context, TYPE_FLOAT, b);
    break;
  }

  case DECLARE_STR: {
    p->param = var(parser, context, TYPE_STRING, b);
    break;
  }

  case DECLARE_BOOL: {
    p->param = var(parser, context, TYPE_BOOL, b);
    break;
  }
  case DECLARE_CHAR: {
    p->param = var(parser,context, TYPE_CHAR,b );
    break;
  }
  case TOKEN_SELF: {
    int self_len = parser->current.length;
    char *self_name = (char *)arena_alloc(context->a, self_len + 1);
    memcpy(self_name, parser->current.start, self_len);
    self_name[self_len] = '\0';
    advance(parser);
    const char *fxn_n = (parser->lexer->fxn && parser->lexer->fxn->name) ? parser->lexer->fxn->name : "";
    const char *underscore = strchr(fxn_n, '_');
    int s_len = underscore ? (int)(underscore - fxn_n) : (int)strlen(fxn_n);
    char *s_type = (char *)arena_alloc(context->a, s_len + 1);
    memcpy(s_type, fxn_n, s_len);
    s_type[s_len] = '\0';

    ASTNode *var_node = create_var_decl_node(
        self_name, self_len, TYPE_STRUCT, NULL, *parser->lexer->fxn,
        parser->lexer->scope_level, context->a, 1, BUCKET_PLUS_ONE, -1);
    var_node->var_decl.struct_type_name = s_type;
    var_node->var_decl.struct_type_name_len = s_len;
    p->param = var_node;
    break;
  }
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

    if (parser->current.type == TOKEN_SELF) {
      advance(parser);
      ASTNode *var_node = create_var_decl_node(
          "self", 4, TYPE_STRUCT, NULL, *parser->lexer->fxn,
          parser->lexer->scope_level, context->a, (ptr_level > 0 ? ptr_level : 1), b, -1);
      var_node->var_decl.struct_type_name = type_name;
      var_node->var_decl.struct_type_name_len = TYPE_LEN;
      p->param = var_node;
    } else if (parser->current.type == TOKEN_IDENTIFIER) {
      Token param_name_token = parser->current;
      advance(parser);
      ASTNode *var_node = create_var_decl_node(
          param_name_token.start, param_name_token.length, TYPE_STRUCT, NULL, *parser->lexer->fxn,
          parser->lexer->scope_level, context->a, ptr_level, b, -1);
      var_node->var_decl.struct_type_name = type_name;
      var_node->var_decl.struct_type_name_len = TYPE_LEN;
      p->param = var_node;
    } else {
      return NULL;
    }
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
    case TYPE_STRING: return (Token){TOKEN_STRING, "", 4, parser->current.line};
    case TYPE_FLOAT:  return (Token){TOKEN_FLOAT, "0.0", 3, parser->current.line};
    case TYPE_BOOL:   return (Token){TOKEN_BOOL, "null", 4, parser->current.line};
    case TYPE_NULL:   return (Token){TOKEN_NULL, "null", 4, parser->current.line};
    case TYPE_CHAR:   return (Token){TOKEN_CHAR, "", 0, parser->current.line};
    case TYPE_INT_ARRAY:
    case TYPE_FLOAT_ARRAY:
    case TYPE_BOOL_ARRAY:
    case TYPE_CHAR_ARRAY:
    case TYPE_STR_ARRAY:
    default:
      return (Token){TOKEN_NULL, "null", 4, parser->current.line};
  }
}

ASTNode *parse_deref_assignment(Parser *parser, CodegenContext *context) {
  int deref_level = 0;
  while (parser->current.type == TOKEN_MUL) {
    deref_level++;
    advance(parser);
  }

  Token var_token = parser->current;
  consume(parser, TOKEN_IDENTIFIER, "Expected identifier after '*'.");

  const int NAME_LENGTH = var_token.length;
  char *potential_var_name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);
  memcpy(potential_var_name, var_token.start, NAME_LENGTH);
  potential_var_name[NAME_LENGTH] = '\0';

  consume(parser, TOKEN_ASSIGN, "Expected '=' after dereferenced variable.");
  ASTNode *value = parse_logical_or(parser, context);
  consume(parser, TOKEN_SEMICOLON,
          "Expected trailing semicolon ';' after assignment.");

  return create_var_assign_node(
      potential_var_name, NAME_LENGTH, value, *parser->lexer->fxn,
      parser->lexer->scope_level, context->a, deref_level);
}

__attribute__((always_inline)) bool is_all_caps(const char *name, int length) {
    if (!name || length <= 0) return false;
    bool has_alpha = false;
    for (int i = 0; i < length; i++) {
        unsigned char c = (unsigned char)name[i];
        if (_IS_LOWER_(c)) return false; 
        if (_IS_UPPER_(c)) has_alpha = true;
    }
    return has_alpha; 
}

__attribute__((always_inline)) int token_to_int(Token token) {
  int result = 0;
  for (int i = 0; i < token.length; i++) {
    result = result * 10 + (token.start[i] - '0');
  }
  return result;
}

void token_to_str(Token token, char* str) {
  memcpy(str, token.start, token.length);
  str[token.length] = '\0';
  
}

