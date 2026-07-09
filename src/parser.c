/*--------------------------------------------------------------------------------

                            SYNTAX CHECKER

---------------------------------------------------------------------------------*/

#include "../headers/ast.h"
#include "../headers/defs.h"
#include "../headers/error.h"
#include "../headers/functions.h"
#include "../headers/token.h"
#include "../headers/variables.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static ASTNode *parse_logical_and(Parser *parser);

static ASTNode *parse_logical_or(Parser *parser);
static ASTNode *parse_body_statement(Parser *parser, CodegenContext *context);
static ASTNode *parse_block(Parser *parser, CodegenContext *context);


// move on to next token
static void advance(Parser *parser) {
  parser->previous = parser->current;
  parser->current = next_token(parser->lexer);
}

// Resynchronizes the parser after an error to avoid cascading false-positive errors
static void synchronize(Parser *parser, TokenType safe_token) {
  advance(parser);
  while (parser->current.type != TOKEN_EOF && parser->current.type != safe_token)advance(parser);
}

static void consume(Parser *parser, TokenType type, const char *errorMessage) {

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



/*------------------ARITHMETICS------------------*/

// 1st Priority
static ASTNode *parse_primary(Parser *parser) {
  Token token = parser->current;

  if (token.type == TOKEN_INT) {
    consume(parser, TOKEN_INT, "Expected integer literal.");
    return create_literal_node(token);
  }

  if (token.type == TOKEN_STRING) {
    consume(parser, TOKEN_STRING, "Expected string literal.");
    return create_literal_node(token);
  }
  if (token.type == TOKEN_BOOL || token.type == TOKEN_NULL) {
    (token.type == TOKEN_BOOL)
        ? consume(parser, TOKEN_BOOL, "Expected boolean literal.")
        : consume(parser, TOKEN_NULL, "Expected null value literal.");
    return create_literal_node(token);
  }

  if (token.type == TOKEN_FLOAT) {
    consume(parser, TOKEN_FLOAT, "Expected float literal.");
    return create_literal_node(token);
  }

  if (token.type == TOKEN_IDENTIFIER) {
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
    return create_var_ref_node(token.start, token.length, *parser->lexer->fxn, parser->lexer->scope_level);
  }

  if (!(parser->current.type == TOKEN_EOF && parser->lexer->errors->size > 0)) {
    error(parser, "Expected expression value.", SYNTAXERROR);
  }
  advance(parser);
  return NULL;
}

// Second Priority
static ASTNode *parse_factor(Parser *parser) {
  ASTNode *left = parse_primary(parser);

  while (parser->current.type == TOKEN_MUL ||
         parser->current.type == TOKEN_DIV ||
         parser->current.type == TOKEN_MOD) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_primary(parser);
    left = create_binary_node(left, operator_type, right);
  }

  return left;
}

// Third Priority
static ASTNode *parse_expression(Parser *parser) {
  ASTNode *left = parse_factor(parser);

  while (parser->current.type == TOKEN_ADD ||
         parser->current.type == TOKEN_SUB) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_factor(parser);
    left = create_binary_node(left, operator_type, right);
  }

  return left;
}

// Fourth Priority
static ASTNode *parse_comparison(Parser *parser) {
  ASTNode *left = parse_expression(parser);

  while (parser->current.type == TOKEN_GT || parser->current.type == TOKEN_ST ||
         parser->current.type == TOKEN_GE || parser->current.type == TOKEN_SE ||
         parser->current.type == TOKEN_EQT ||
         parser->current.type == TOKEN_NEQ) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_expression(parser);
    left = create_binary_node(left, operator_type, right);
  }

  return left;
}

// Fifth Priority
static ASTNode *parse_logical_and(Parser *parser) {
  ASTNode *left = parse_comparison(parser);
  while (parser->current.type == TOKEN_AND) {
    TokenType operator_type = parser->current.type;
    advance(parser);
    ASTNode *right = parse_comparison(parser);
    left = create_binary_node(left, operator_type, right);
  }
  return left;
}

// High priority
static ASTNode *parse_logical_or(Parser *parser) {
  ASTNode *left = parse_logical_and(parser);
  while (parser->current.type == TOKEN_OR) {
    TokenType operator_type = parser->current.type;
    advance(parser);
    ASTNode *right = parse_logical_and(parser);
    left = create_binary_node(left, operator_type, right);
  }
  return left;
}

// infer_expr_type moved to semantic.c

static ASTNode *println(Parser *parser, CodegenContext *context) {
  advance(parser); // Move past TOKEN_PRINTLN
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' for arguments.");

  // Parse whatever is inside the parentheses as a unified expression
  ASTNode *expr = parse_logical_or(parser);

  ASTNode *println_node = create_println_node(expr);

  consume(parser, TOKEN_RPARETH,
          "Expected close parenthesis ')' after arguments");
  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
  return println_node;
}

ASTNode *var(Parser *parser, CodegenContext *context) {
  advance(parser);

  set declaring = false;

  Token name_token = parser->current;
  const int NAME_LENGTH = name_token.length;
  char name[NAME_LENGTH + 1];
  snprintf(name, sizeof(name), "%.*s", NAME_LENGTH, name_token.start);

  ASTNode *value;

  consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");

  if (parser->current.type == TOKEN_SEMICOLON) {
    declaring = true;
    Token null_token = {TOKEN_NULL, "null", 4, parser->current.line};
    value = create_literal_node(null_token);
  }

  if (!declaring) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");
    value = parse_logical_or(parser);
  }


  ASTNode *var_node = create_var_decl_node(name_token.start, name_token.length,
                                           TYPE_NULL, value, *parser->lexer->fxn, parser->lexer->scope_level);

  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
  return var_node;
}

static ASTNode *parse_assignment_or_increment(Parser *parser, CodegenContext *context, char *potential_var_name, int NAME_LENGTH) {
  ASTNode *value;
  set incrementation = false;

  TokenType alternatives[] = {TOKEN_AEQ, TOKEN_SEQ, TOKEN_MEQ, TOKEN_DEQ, TOKEN_PEQ};

  if (parser->current.type == TOKEN_INC || parser->current.type == TOKEN_DEC) {
    incrementation = true;
    TokenType op = parser->current.type == TOKEN_INC ? TOKEN_ADD : TOKEN_SUB;
    ASTNode *var_ref = create_var_ref_node(potential_var_name, NAME_LENGTH, *parser->lexer->fxn, parser->lexer->scope_level);
    Token one_token = {.type = TOKEN_INT, .start = "1", .length = 1, .line = parser->current.line};
    ASTNode *literal_one = create_literal_node(one_token);
    value = create_binary_node(var_ref, op, literal_one);
    advance(parser);
  }

  set is_alternative = false;
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
      case TOKEN_AEQ: op = TOKEN_ADD; break;
      case TOKEN_SEQ: op = TOKEN_SUB; break;
      case TOKEN_MEQ: op = TOKEN_MUL; break;
      case TOKEN_DEQ: op = TOKEN_DIV; break;
      case TOKEN_PEQ: op = TOKEN_MOD; break;
    }
    ASTNode *var_ref = create_var_ref_node(potential_var_name, NAME_LENGTH, *parser->lexer->fxn, parser->lexer->scope_level);
    advance(parser);
    ASTNode *left = parse_logical_or(parser);
    value = create_binary_node(var_ref, op, left);
  }

  if (!incrementation) {
    consume(parser, TOKEN_ASSIGN, "Expected '=' after variable.");
    value = parse_logical_or(parser);
  }

  return create_var_assign_node(potential_var_name, NAME_LENGTH, value, *parser->lexer->fxn, parser->lexer->scope_level);
}

static ASTNode *parse_fxn_call(Parser *parser, CodegenContext *context,
                               char *name, int name_length) {
  consume(parser, TOKEN_LPARETH, "Expected '(' after function name.");
  consume(parser, TOKEN_RPARETH, "Expected ')' after arguments.");
  return create_fxn_call_node(name, name_length, TYPE_NULL, *parser->lexer->fxn, parser->lexer->scope_level);
}

static ASTNode *identifier(Parser *parser, CodegenContext *context) {
  const int NAME_LENGTH = parser->current.length;
  char *potential_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
  sprintf(potential_name, "%.*s", NAME_LENGTH,
           parser->current.start);
           
  advance(parser);

  if (parser->current.type == TOKEN_LPARETH) {
    ASTNode *call_node = parse_fxn_call(parser, context, potential_name, NAME_LENGTH);
    consume(parser, TOKEN_SEMICOLON,
            "Expected trailing semicolon ';' after function call.");
    return call_node;
  }

  ASTNode *assign_var = parse_assignment_or_increment(parser, context, potential_name, NAME_LENGTH);
  if (!assign_var) {
    return NULL;
  }
  consume(parser, TOKEN_SEMICOLON,
          "Expected trailing semicolon ';' to terminate statement.");
  return assign_var;
}

static ASTNode *parse_block(Parser *parser, CodegenContext *context);

ASTNode *parse_condition(Parser *parser, CodegenContext *context) {

  if (parser->current.type == TOKEN_WHILE) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('.");
    ASTNode *condition = parse_logical_or(parser);
    consume(parser, TOKEN_RPARETH, "Expected ')'.");
    ASTNode *then_block = parse_block(parser, context);

    return create_while_node(condition, then_block);
  }

  if (parser->current.type == TOKEN_FOR) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('");
    ASTNode *variable = var(parser, context);

    ASTNode *condition = parse_logical_or(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition.");
    
    const int NAME_LENGTH = parser->current.length;
    char *loop_var_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
    sprintf(loop_var_name, "%.*s", NAME_LENGTH, parser->current.start);
    advance(parser);
    ASTNode *var_operation = parse_assignment_or_increment(parser, context, loop_var_name, NAME_LENGTH);
    consume(parser, TOKEN_RPARETH, "Expected ')'");

    ASTNode *then_block = parse_block(parser, context);
    return create_for_node(variable, condition, var_operation, then_block);
  }
  if (parser->current.type == TOKEN_IF || parser->current.type == TOKEN_ELIF) {
    advance(parser);
    consume(parser, TOKEN_LPARETH, "Expected '('.");
    ASTNode *condition = parse_logical_or(parser);
    consume(parser, TOKEN_RPARETH, "Expected ')'.");

    ASTNode *then_block = parse_block(parser, context);

    ASTNode *else_block = NULL;
    if (parser->current.type == TOKEN_ELIF) {
      else_block = parse_condition(parser, context);
    } else if (parser->current.type == TOKEN_ELSE) {
      consume(parser, TOKEN_ELSE, "Expected 'else'.");
      else_block = parse_block(parser, context);
    }

    return create_if_node(condition, then_block, else_block);
  }
  advance(parser);
}

// check the body statement
static ASTNode *parse_body_statement(Parser *parser, CodegenContext *context) {
  switch (parser->current.type) {

  case TOKEN_PRINTLN:
    return println(parser, context);

  case TOKEN_VAR:
    return var(parser, context);

  case TOKEN_NULL:
  case TOKEN_IDENTIFIER:
    return identifier(parser, context);

  case TOKEN_FOR:
  case TOKEN_WHILE:
  case TOKEN_IF:
    return parse_condition(parser, context);

  default:
    error(parser, "Unrecognized token in body statement.", SYNTAXERROR);
    synchronize(parser, TOKEN_RBRACE);
    return NULL;
  }
}

// parse_block() checks the innard of that function.
static ASTNode *parse_block(Parser *parser, CodegenContext *context) {
  consume(parser, TOKEN_LBRACE, "Expected open brace '{' to begin block.");

  ASTNode *block = create_block_node();

  while (parser->current.type != TOKEN_RBRACE &&
         parser->current.type != TOKEN_EOF) {
    ASTNode *stmt = parse_body_statement(parser, context);
    if (stmt != NULL) {
      block_add_statement(block, stmt);
    }
  }

  consume(parser, TOKEN_RBRACE,
          "Expected closing brace '}' to terminate block.");
  return block;
}

ASTNode *function(Parser *parser, CodegenContext *context) {
  advance(parser);
  consume(parser, TOKEN_LPARETH,
          "Expected parameter list wrapper starting with '('.");
  consume(parser, TOKEN_RPARETH,
          "Expected closing parameter list wrapper ')'.");

  consume(parser, TOKEN_ARROW, "Expected return signature pointer token '->'.");
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' around return type specification.");
  consume(parser, TOKEN_VOID,
          "Expected explicit type parameter keyword 'void'.");
  consume(parser, TOKEN_RPARETH,
          "Expected closing parenthesis ')' around return type specification.");

  ASTNode *body = parse_block(parser, context);

  int name_length = strlen(parser->lexer->fxn->name);
  printf("REGISTERING FXN: %s\n", parser->lexer->fxn->name);
  return create_function_node(parser->lexer->fxn->name, name_length, body, *parser->lexer->fxn, parser->lexer->scope_level);
}

ASTNode *register_and_form_fxn(Parser *parser, CodegenContext *context) {
  
  Token name_token = parser->current;

  const int NAME_LENGTH = name_token.length;
  char *name = alloc_space(NAME_LENGTH + 1 , sizeof(char));
  sprintf(name, "%.*s", NAME_LENGTH, name_token.start);

  //FXN fxn = {NAME_LENGTH, parser->lexer->line, parser->lexer->scope_level, TYPE_NULL, name , parser->lexer->fxn};

  FXN *fxn =  (FXN*)alloc_space(1, sizeof(FXN));

  fxn->length = NAME_LENGTH;
  fxn->name = name;
  fxn->level = parser->lexer->scope_level;
  fxn->line = parser->lexer->line;
  fxn->parent_fxn = parser->lexer->fxn;
  fxn->return_type = TYPE_NULL;
  *parser->lexer->fxn = *fxn;

  return function(parser, context);
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

ASTNode *begin(Parser *parser, CodegenContext *context) {
  *parser->lexer->fxn = (FXN) {0, 0, 0, TYPE_NULL, "global", NULL};
  ASTNode *program_block = create_block_node();
  
  while (parser->current.type == TOKEN_FXN) {
    ASTNode *fxn_node = parse_function(parser, context);
    if (fxn_node) {
      block_add_statement(program_block, fxn_node);
    }
  }
  
  return create_program_node(program_block);
}

ASTNode *compile_parse(Lexer *lexer, errorStack *s, CodegenContext *context) {
  Parser parser = {0};
  parser.lexer = lexer;

  parser.error = s;
  // Prime by fetching the first token. That way parser.current is not NULL
  advance(&parser);

  // Now parsing the function while sharing the symbol context.
  ASTNode *program_node = begin(&parser, context);

  // Checks the end of the file.
  consume(
      &parser, TOKEN_EOF,
      "Unexpected trailing syntax tokens encountered after main entry block.");

  printf("Checking errors before compilation...\n\n");
  
  return program_node;
}

/*--------------------------------------------------------------------------------

                                    HELPERS

---------------------------------------------------------------------------------*/


