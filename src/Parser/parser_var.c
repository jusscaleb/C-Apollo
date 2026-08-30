#include "../../headers/parser.h"

ASTNode *var(Parser *parser, CodegenContext *context, DataType var_type,
             MemoryBucket bucket) {
  advance(parser);

  int pointer_level = 0;
  DataType dt = var_type;
  MemoryBucket final_bucket = bucket;

  while (parser->current.type == TOKEN_MUL) {
    pointer_level++;
    advance(parser);
  }
  bool is_constant = is_all_caps(parser->current.start, parser->current.length);
  if (is_constant && final_bucket == BUCKET_THREE)
    error(parser, "Heap variables (@) cannot be declared as constants",
          SYNTAXERROR);

  if (is_constant)
    final_bucket = BUCKET_ONE;
  if (pointer_level > 0 && is_constant)
    error(parser, "Cannot assign memory address to constant", SYNTAXERROR);

  ASTNode *value;
  bool declaring = false, decl_param = false;
  Token name_token;
  int arr_length = -1;

  if (parser->current.type == TOKEN_LSQUARE_BRACE) {
    switch (dt) {
    case TYPE_INT:
      dt = TYPE_INT_ARRAY;
      break;
    case TYPE_FLOAT:
      dt = TYPE_FLOAT_ARRAY;
      break;
    case TYPE_BOOL:
      dt = TYPE_BOOL_ARRAY;
      break;
    case TYPE_CHAR:
      dt = TYPE_CHAR_ARRAY;
      break;
    case TYPE_STRING:
      dt = TYPE_STR_ARRAY;
      break;
    default:
      break;
    }

    advance(parser);
    if (parser->current.type != TOKEN_RSQUARE_BRACE) {
      final_bucket = (bucket == BUCKET_THREE) ? BUCKET_THREE : BUCKET_PLUS_ONE;
      if (parser->current.type != TOKEN_INT) {
        error(parser, "Expected fixed array length or ']'", SYNTAXERROR);
      }

      arr_length = token_to_int(parser->current);
      advance(parser);
    } else {
      final_bucket = (bucket == BUCKET_THREE) ? BUCKET_THREE : BUCKET_TWO;
    }
    consume(parser, TOKEN_RSQUARE_BRACE, "Expected closing ']'");
  }

  name_token = parser->current;
  consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");

  if (parser->current.type == TOKEN_SEMICOLON) {
    if (final_bucket == BUCKET_ONE)
      error(parser, "Assign a value to constant.", SYNTAXERROR);

    declaring = true;
    Token var_decl_token = parser_get_var_token(parser, dt);
    value = create_literal_node(var_decl_token, context->a);
  }

  if (parser->current.type == TOKEN_COMMA ||
      (parser->current.type == TOKEN_RPARETH && dt != TYPE_NULL &&
          !declaring)) {
    decl_param = true;
    Token var_decl_token = parser_get_var_token(parser, dt);
    value = create_literal_node(var_decl_token, context->a);
  }

  if (!declaring && !decl_param) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");
    if (parser->current.type == TOKEN_LBRACE) {
      value = parse_array_node(parser, dt, context, arr_length);
    } else {
      value = parse_logical_or(parser, context);
    }
  }

  ASTNode *var_node = create_var_decl_node(
      name_token.start, name_token.length, dt, value, *parser->lexer->fxn,
      parser->lexer->scope_level, context->a, pointer_level, final_bucket,
      arr_length);

  return var_node;
}

ASTNode *parse_assignment_or_increment(Parser *parser, CodegenContext *context,
                                       char *potential_var_name,
                                       int NAME_LENGTH) {
  ASTNode *value;
  bool incrementation = false;

  TokenType alternatives[] = {TOKEN_AEQ, TOKEN_SEQ, TOKEN_MEQ, TOKEN_DEQ,
                              TOKEN_PEQ};

  if (parser->current.type == TOKEN_INC || parser->current.type == TOKEN_DEC) {
    incrementation = true;
    TokenType op = parser->current.type == TOKEN_INC ? TOKEN_ADD : TOKEN_SUB;
    ASTNode *var_ref = create_var_ref_node(
        potential_var_name, NAME_LENGTH, *parser->lexer->fxn,
        parser->lexer->scope_level, context->a);
    Token one_token = {.type = TOKEN_INT,
                       .start = "1",
                       .length = 1,
                       .line = parser->current.line};
    ASTNode *literal_one = create_literal_node(one_token, context->a);
    value = create_binary_node(var_ref, op, literal_one, context->a);
    advance(parser);
  }

  bool is_alternative = false;
  for (int i = 0; i <= 4; i++) {
    if (parser->current.type == alternatives[i]) {
      is_alternative = true;
      break;
    }
  }

  if (is_alternative && !incrementation) {
    incrementation = true;
    TokenType op;
    switch (parser->current.type) {
    case TOKEN_AEQ:
      op = TOKEN_ADD;
      break;
    case TOKEN_SEQ:
      op = TOKEN_SUB;
      break;
    case TOKEN_MEQ:
      op = TOKEN_MUL;
      break;
    case TOKEN_DEQ:
      op = TOKEN_DIV;
      break;
    case TOKEN_PEQ:
      op = TOKEN_MOD;
      break;
    default:
      error(parser, "Unrecognized token", SYNTAXERROR);
      break;
    }
    ASTNode *var_ref = create_var_ref_node(
        potential_var_name, NAME_LENGTH, *parser->lexer->fxn,
        parser->lexer->scope_level, context->a);
    advance(parser);
    ASTNode *left = parse_logical_or(parser, context);
    value = create_binary_node(var_ref, op, left, context->a);
  }

  if (!incrementation) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after variable.");
    value = parse_logical_or(parser, context);
  }

  return create_var_assign_node(potential_var_name, NAME_LENGTH, value,
                                *parser->lexer->fxn, parser->lexer->scope_level,
                                context->a, 0);
}

ASTNode *identifier(Parser *parser, CodegenContext *context) {
  const int NAME_LENGTH = parser->current.length;
  char *potential_name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);
  memcpy(potential_name, parser->current.start, NAME_LENGTH);
  potential_name[NAME_LENGTH] = '\0';

  advance(parser);
  if (parser->current.type == TOKEN_LSQUARE_BRACE) {
    ASTNode *var_ref =
        create_var_ref_node(potential_name, NAME_LENGTH, *parser->lexer->fxn,
                            parser->lexer->scope_level, context->a);
    ASTNode *expr_index = parse_index_expr(parser, context, var_ref);
    consume(parser, TOKEN_SEMICOLON,
            "Expected trailing semicolon ';' after expression statement.");
    return expr_index;
  }
  if (parser->current.type == TOKEN_LPARETH) {
    ASTNode *call_node =
        parse_fxn_call(parser, context, potential_name, NAME_LENGTH);

    if (parser->current.type == TOKEN_LSQUARE_BRACE) {
      call_node = parse_index_expr(parser, context, call_node);
    }

    consume(parser, TOKEN_SEMICOLON,
            "Expected trailing semicolon ';' after function call.");

    return call_node;
  }

  ASTNode *assign_var = parse_assignment_or_increment(
      parser, context, potential_name, NAME_LENGTH);
  if (!assign_var) {
    return NULL;
  }
  consume(parser, TOKEN_SEMICOLON,
          "Expected trailing semicolon ';' to terminate statement.");
  return assign_var;
}

ASTNode *parse_condition(Parser *parser, CodegenContext *context) {

  if (parser->current.type == TOKEN_WHILE) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('.");
    ASTNode *condition = parse_logical_or(parser, context);
    consume(parser, TOKEN_RPARETH, "Expected ')'.");
    ASTNode *then_block = parse_block(parser, context);

    return create_while_node(condition, then_block, context->a);
  }

  if (parser->current.type == TOKEN_FOR) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('");
    ASTNode *variable = var(parser, context, TYPE_NULL, 0);
    consume(parser, TOKEN_SEMICOLON,
            "Expected ';' after for-loop variable initialization.");
    ASTNode *condition = parse_logical_or(parser, context);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after for-loop condition.");

    const int NAME_LENGTH = parser->current.length;
    char *loop_var_name = arena_alloc(context->a, NAME_LENGTH + 1);
    memcpy(loop_var_name, parser->current.start, NAME_LENGTH);
    loop_var_name[NAME_LENGTH] = '\0';
    advance(parser);
    ASTNode *var_operation = parse_assignment_or_increment(
        parser, context, loop_var_name, NAME_LENGTH);
    consume(parser, TOKEN_RPARETH, "Expected ')'");

    ASTNode *then_block = parse_block(parser, context);
    return create_for_node(variable, condition, var_operation, then_block,
                           context->a);
  }
  if (parser->current.type == TOKEN_IF || parser->current.type == TOKEN_ELIF) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('.");
    ASTNode *condition = parse_logical_or(parser, context);
    consume(parser, TOKEN_RPARETH, "Expected ')'.");

    ASTNode *then_block = parse_block(parser, context);

    ASTNode *else_block = NULL;
    if (parser->current.type == TOKEN_ELIF) {
      else_block = parse_condition(parser, context);
    } else if (parser->current.type == TOKEN_ELSE) {
      consume(parser, TOKEN_ELSE, "Expected 'else'.");
      else_block = parse_block(parser, context);
    }

    return create_if_node(condition, then_block, else_block, context->a);
  }
  advance(parser);
}

// check the body statement
ASTNode *parse_body_statement(Parser *parser, CodegenContext *context) {
  switch (parser->current.type) {
  case TOKEN_VAR:
  case DECLARE_INT:
  case DECLARE_STR:
  case DECLARE_FLOAT:
  case DECLARE_BOOL:
  case DECLARE_CHAR: {
    MemoryBucket b =
        (parser->previous.type == TOKEN_SIGIL) ? BUCKET_THREE : BUCKET_PLUS_ONE;
    DataType dt = TYPE_NULL;
    switch (parser->current.type) {
    case DECLARE_BOOL:
      dt = TYPE_BOOL;
      break;
    case DECLARE_FLOAT:
      dt = TYPE_FLOAT;
      break;
    case DECLARE_STR:
      dt = TYPE_STRING;
      break;
    case DECLARE_INT:
      dt = TYPE_INT;
      break;

    case DECLARE_CHAR:
      dt = TYPE_CHAR;
      break;
    default:
      break;
    }

    ASTNode *var_node = var(parser, context, dt, b);
    consume(parser, TOKEN_SEMICOLON, "Expected semi-colon to end statement.");
    return var_node;
  }

  case TOKEN_MUL: {
    return parse_deref_assignment(parser, context);
  }

  case TOKEN_SIGIL: {
    advance(parser);

    if (parser->current.type != TOKEN_VAR &&
        parser->current.type != DECLARE_BOOL &&
        parser->current.type == DECLARE_FLOAT &&
        parser->current.type != DECLARE_INT &&
        parser->current.type != DECLARE_STR)
      error(parser, "Expected variable declaration after '@'.", SYNTAXERROR);

    return parse_body_statement(parser, context);
  }

  case TOKEN_NULL:
  case TOKEN_IDENTIFIER:
    return identifier(parser, context);

  case TOKEN_FOR:
  case TOKEN_WHILE:
  case TOKEN_IF:
    return parse_condition(parser, context);

  case TOKEN_FXN:
    return parse_function(parser, context);

  case TOKEN_RETURN:
    advance(parser);
    parser->lexer->fxn->ret_nodes++;
    Fxn *fxn = (Fxn *)arena_alloc(context->a, sizeof(Fxn));
    fxn = parser->lexer->fxn;
    ASTNode *value;
    if (parser->current.type != TOKEN_LBRACE) {
      value = parse_logical_or(parser, context);
    } else {
      value =
          parse_array_node(parser, fxn->return_type, context, fxn->array_count);
    }
    ASTNode *ret_node = create_ret_node(context, *fxn, value, context->a);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after return statement");
    return ret_node;

  default:
    error(parser, "Unrecognized token in body statement.", SYNTAXERROR);
    synchronize(parser, TOKEN_RBRACE);
    return NULL;
  }
}

// parse_block() checks the innard of that function.
