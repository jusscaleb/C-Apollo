#include "../../headers/parser.h"

ASTNode *var(Parser *parser, CodegenContext *context, DataType var_type) {
  advance(parser);
  DataType dt = var_type;

  bool declaring = false, decl_param = false;

  Token name_token = parser->current;
  const int NAME_LENGTH = name_token.length;
  char name[NAME_LENGTH + 1];
  memcpy(name, name_token.start, NAME_LENGTH);
  name[NAME_LENGTH] = '\0';

  ASTNode *value;

  consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");
  if (parser->current.type == TOKEN_SEMICOLON) {
    declaring = true;
    Token var_decl_token = parser_get_var_token(parser, dt);

    value = create_literal_node(var_decl_token, context->a);
  }

  if (parser->current.type == TOKEN_COMMA ||
      parser->current.type == TOKEN_RPARETH && dt != TYPE_NULL && !declaring) {
    decl_param = true;
    Token var_decl_token = parser_get_var_token(parser, dt);
    value = create_literal_node(var_decl_token, context->a);
  }

  if (!declaring && !decl_param) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");
    value = parse_logical_or(parser, context);
  }

  ASTNode *var_node =
      create_var_decl_node(name_token.start, name_token.length, dt, value,
                           *parser->lexer->fxn, parser->lexer->scope_level, context->a);

  return var_node;
}

ASTNode *parse_assignment_or_increment(Parser *parser,
                                              CodegenContext *context,
                                              char *potential_var_name,
                                              int NAME_LENGTH) {
  ASTNode *value;
  bool incrementation = false;

  TokenType alternatives[] = {TOKEN_AEQ, TOKEN_SEQ, TOKEN_MEQ, TOKEN_DEQ,
                              TOKEN_PEQ};

  if (parser->current.type == TOKEN_INC || parser->current.type == TOKEN_DEC) {
    incrementation = true;
    TokenType op = parser->current.type == TOKEN_INC ? TOKEN_ADD : TOKEN_SUB;
    ASTNode *var_ref =
        create_var_ref_node(potential_var_name, NAME_LENGTH,
                            *parser->lexer->fxn, parser->lexer->scope_level, context->a);
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
    ASTNode *var_ref =
        create_var_ref_node(potential_var_name, NAME_LENGTH,
                            *parser->lexer->fxn, parser->lexer->scope_level, context->a);
    advance(parser);
    ASTNode *left = parse_logical_or(parser, context);
    value = create_binary_node(var_ref, op, left, context->a);
  }

  if (!incrementation) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after variable.");
    value = parse_logical_or(parser, context);
  }

  return create_var_assign_node(potential_var_name, NAME_LENGTH, value,
                                *parser->lexer->fxn,
                                parser->lexer->scope_level, context->a);
}


ASTNode *identifier(Parser *parser, CodegenContext *context) {
  const int NAME_LENGTH = parser->current.length;
  char *potential_name = (char*)arena_alloc(context->a, NAME_LENGTH+1);
  memcpy(potential_name, parser->current.start, NAME_LENGTH);
  potential_name[NAME_LENGTH] = '\0';

  advance(parser);

  if (parser->current.type == TOKEN_LPARETH) {
    ASTNode *call_node =
        parse_fxn_call(parser, context, potential_name, NAME_LENGTH);
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
    ASTNode *variable = var(parser, context, TYPE_NULL);
    consume(parser, TOKEN_SEMICOLON,
            "Expected ';' after for-loop variable initialization.");
    ASTNode *condition = parse_logical_or(parser, context);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after for-loop condition.");

    const int NAME_LENGTH = parser->current.length;
    char *loop_var_name = arena_alloc(context->a, NAME_LENGTH+1);
    memcpy(loop_var_name, parser->current.start, NAME_LENGTH);
    loop_var_name[NAME_LENGTH] = '\0';
    advance(parser);
    ASTNode *var_operation = parse_assignment_or_increment(
        parser, context, loop_var_name, NAME_LENGTH);
    consume(parser, TOKEN_RPARETH, "Expected ')'");

    ASTNode *then_block = parse_block(parser, context);
    return create_for_node(variable, condition, var_operation, then_block, context->a);
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
    default:
      break;
    }

    ASTNode *var_node = var(parser, context, dt);
    consume(parser, TOKEN_SEMICOLON, "Expected After Variable declaration.");
    return var_node;

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
    if(!parser->lexer->fxn->has_return_type) parser->lexer->fxn->has_return_type = true;
    Fxn *fxn = (Fxn *)arena_alloc(context->a, sizeof(Fxn));
    fxn = parser->lexer->fxn;
    ASTNode *value = parse_logical_or(parser, context);
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
