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
#include <stdlib.h>
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
  advance(parser);
}

// Resynchronizes the parser after an error to avoid cascading false-positive errors
static void synchronize(Parser *parser) {
  advance(parser);

  while (parser->current.type != TOKEN_EOF) {
    if (parser->previous.type == TOKEN_SEMICOLON) return;

    switch (parser->current.type) {
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINTLN:
      case TOKEN_FXN:
        return;
      default:
        // Keep advancing until a statement boundary is found
        ;
    }

    advance(parser);
  }
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
    return create_var_ref_node(token.start, token.length);
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

static datatype infer_expr_type(Parser *parser, CodegenContext *context,
                                ASTNode *expr) {
  if (expr->Type == AST_LITERAL_EXPR) {
    if (expr->literal_expr.token.type == TOKEN_FLOAT)
      return TYPE_FLOAT;
    if (expr->literal_expr.token.type == TOKEN_BOOL)
      return TYPE_BOOL;
    if (expr->literal_expr.token.type == TOKEN_NULL)
      return TYPE_NULL;
    return TYPE_INT;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    datatype left_type =
        infer_expr_type(parser, context, expr->binary_expr.left);
    datatype right_type =
        infer_expr_type(parser, context, expr->binary_expr.right);

    return (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT
                                                                 : TYPE_INT;
  }

  if (expr->Type == AST_VAR_REF) {
    // char var_name[64];
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[NAME_LENGTH + 1];
    snprintf(var_name, sizeof(var_name), "%.*s", NAME_LENGTH,
             expr->var_ref.name);
    Symbol *sym = lookup_token(context, var_name);

    if (sym) {
      return sym->type;
    }
  }

  error(parser, "Could not infer expression type.", TYPEERROR);
  return TYPE_NULL;
}

static ASTNode *println(Parser *parser, CodegenContext *context) {
  advance(parser); // Move past TOKEN_PRINTLN
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' for arguments.");

  // Parse whatever is inside the parentheses as a unified expression
  ASTNode *expr = parse_logical_or(parser);

  ASTNode *println_node = create_println_node(expr);

  consume(parser, TOKEN_RPARETH,
          "Expected close parenthesis ')' after arguments. Found [%c]");
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

  Symbol *sym = lookup_token(context, name);
  if (sym) {
    error(parser, "Multiple definition of variable.", REFERROR);
    advance(parser);
  }

  Variable variable;
  ASTNode *value;

  consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");

  if (parser->current.type == TOKEN_SEMICOLON) {
    declaring = true;
    Token null_token = {TOKEN_NULL, "null", 4, parser->current.line};
    value = create_literal_node(null_token);

    variable = (Variable){TYPE_NULL, "null", 4, parser->current.line};
    register_variable(context, name, variable.type);
  }

  if (!declaring)
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");

  switch (parser->current.type) {

  case TOKEN_INT:
  case TOKEN_FLOAT:
  case TOKEN_IDENTIFIER: {
    value = parse_logical_or(parser);
    datatype inferred = infer_expr_type(parser, context, value);
    variable = (Variable){inferred, NULL, 0, parser->current.line};
    register_variable(context, name, variable.type);
    break;
  }

  case TOKEN_STRING: {
    Token value_token = parser->current;
    variable = (Variable){TYPE_STRING, parser->current.start,
                          parser->current.length, parser->current.line};
    consume(parser, TOKEN_STRING,
            "Expected string literal for variable assignment.");
    value = create_literal_node(value_token);
    register_variable(context, name, variable.type);
    break;
  }

  case TOKEN_NULL:
  case TOKEN_BOOL: {
    Token value_token = parser->current;
    variable = (parser->current.type == TOKEN_BOOL)
                   ? (Variable){TYPE_BOOL, parser->current.start,
                                parser->current.length, parser->current.line}
                   : (Variable){TYPE_NULL, parser->current.start,
                                parser->current.length, parser->current.line};

    (parser->current.type == TOKEN_BOOL)
        ? consume(parser, TOKEN_BOOL,
                  "Expected boolean literal for variable assignment.")
        : consume(parser, TOKEN_NULL, "Expected null literal");

    value = create_literal_node(value_token);
    register_variable(context, name, variable.type);
    break;
  }

  default: {

    if (!declaring) {
      error(parser, "Expected a variable value", SYNTAXERROR);
      advance(parser);
    }
  }
  }

  ASTNode *var_node = create_var_decl_node(name_token.start, name_token.length,
                                           variable.type, value);

  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
  return var_node;
}

static ASTNode *parse_assignment_or_increment(Parser *parser,
                                              CodegenContext *context) {
  ASTNode *value;
  set incrementation = false;

  TokenType alternatives[] = {TOKEN_AEQ, TOKEN_SEQ, TOKEN_MEQ, TOKEN_DEQ,
                              TOKEN_PEQ};

  const int NAME_LENGTH = parser->current.length;
  char potential_var_name[NAME_LENGTH + 1];

  snprintf(potential_var_name, sizeof(potential_var_name), "%.*s", NAME_LENGTH,
           parser->current.start);
  Symbol *sym = lookup_token(context, potential_var_name);

  if (!sym) {
    error(parser, "Unrecognized Token.", REFERROR);
    synchronize(parser);
    return NULL;
  }

  advance(parser);

  if (parser->current.type == TOKEN_INC || parser->current.type == TOKEN_DEC) {
    if (sym->type == TYPE_INT || sym->type == TYPE_FLOAT) {
      incrementation = true;
      TokenType op = parser->current.type == TOKEN_INC ? TOKEN_ADD : TOKEN_SUB;

      ASTNode *var_ref =
          create_var_ref_node(sym->llvm_name, strlen(sym->llvm_name));
      Token one_token = {.type = TOKEN_INT,
                         .start = "1",
                         .length = 1,
                         .line = parser->current.line};
      ASTNode *literal_one = create_literal_node(one_token);

      value = create_binary_node(var_ref, op, literal_one);
      advance(parser);

    } else {
      error(parser, "Needs to be of type float or int for incrementation.",
            TYPEERROR);
      advance(parser);
    }
  }

  set is_alternative = false;

  for (int i = 0; i <= 4; i++) {
    if (parser->current.type == alternatives[i]) {
      is_alternative = true;
      break;
    }
  }

  if (is_alternative) {
    if (sym->type == TYPE_INT || sym->type == TYPE_FLOAT) {
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
      }

      ASTNode *var_ref =
          create_var_ref_node(sym->llvm_name, strlen(sym->llvm_name));
      advance(parser);
      ASTNode *left = create_literal_node(parser->current);

      value = create_binary_node(var_ref, op, left);
      advance(parser);

    } else {
      error(parser, "Needs to be of type float or int for Operation.",
            TYPEERROR);
    }
  }

  if (!incrementation) {

    consume(parser, TOKEN_ASSIGN, "Expected '=' after variable.");

    Token t = parser->current;
    switch (parser->current.type) {
    case TOKEN_STRING: {
      if (sym->type != TYPE_STRING && sym->type != TYPE_NULL) {
        error(parser, "Incompatible assignment type. Expected String.",
              TYPEERROR);
        advance(parser);
      }
      value = create_literal_node(t);
      advance(parser);
      break;
    }

    case TOKEN_INT: {
      if (sym->type != TYPE_INT && sym->type != TYPE_NULL) {
        error(parser, "Incompatible assignment type. Expected Integer.",
              TYPEERROR);
        advance(parser);
      }
      value = parse_logical_or(parser);
      break;
    }

    case TOKEN_FLOAT: {
      if (sym->type != TYPE_FLOAT && sym->type != TYPE_NULL) {
        error(parser, "Incompatible assignment type. Expected Float.",
              TYPEERROR);
        advance(parser);
      }
      value = parse_logical_or(parser);
      break;
    }

    case TOKEN_BOOL: {
      if (sym->type != TYPE_BOOL && sym->type != TYPE_NULL) {
        error(parser, "Incompatible assignment type. Expected Boolean.",
              TYPEERROR);
        advance(parser);
      }
      value = create_literal_node(t);
      advance(parser);
      break;
    }

    case TOKEN_IDENTIFIER: {
      const int NAME_LENGTH = parser->current.length;
      char id_name[NAME_LENGTH + 1];
      sprintf(id_name, "%.*s", NAME_LENGTH, parser->current.start);
      Symbol *var = lookup_token(context, id_name);
      if (!var) {
        error(parser, "Unrecognized token. line 446", REFERROR);
        advance(parser);

      } else if (var->type != sym->type && sym->type != TYPE_NULL) {
        error(parser, "Incompatible assignment type.", TYPEERROR);
        advance(parser);
      }

      value = parse_logical_or(parser);

      break;
    }

    default: {
      error(parser, "Invalid assignment type.", TYPEERROR);
      advance(parser);
    }
    }
  }

  return create_var_assign_node(sym->llvm_name, strlen(sym->llvm_name), value);
}

static ASTNode *parse_fxn_call(Parser *parser, CodegenContext *context,
                               Symbol *sym) {
  advance(parser); // Move past function name identifier
  consume(parser, TOKEN_LPARETH, "Expected '(' after function name.");
  consume(parser, TOKEN_RPARETH, "Expected ')' after arguments.");
  return create_fxn_call_node((char *)sym->name, strlen(sym->name), sym->type);
}

static ASTNode *identifier(Parser *parser, CodegenContext *context) {
  const int NAME_LENGTH = parser->current.length;
  char potential_name[NAME_LENGTH + 1];
  snprintf(potential_name, sizeof(potential_name), "%.*s", NAME_LENGTH,
           parser->current.start);
  Symbol *sym = lookup_token(context, potential_name);
  if (sym && sym->t_type == FUNC) {
    ASTNode *call_node = parse_fxn_call(parser, context, sym);
    consume(parser, TOKEN_SEMICOLON,
            "Expected trailing semicolon ';' after function call.");
    return call_node;
  }

  ASTNode *assign_var = parse_assignment_or_increment(parser, context);
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

    if (variable->Type == TYPE_STRING || variable->Type == TYPE_BOOL) {
      error(parser, "For loop variable must be int or float.", TYPEERROR);
      advance(parser);
    }

    ASTNode *condition = parse_logical_or(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition.");
    ASTNode *var_operation = parse_assignment_or_increment(parser, context);
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
    synchronize(parser);
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

void function(Parser *parser, CodegenContext *context, char *name) {
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

  gen_function_start(context, name);

  ASTNode *body = parse_block(parser, context);

  if (parser->lexer->errors->size == 0) {
    gen_block_from_ast(context, body);
  }

  gen_function_end(context, strcmp(name, "run") == 0);
}

void register_and_form_fxn(Parser *parser, CodegenContext *context) {
  Token name_token = parser->current;

  const int NAME_LENGTH = name_token.length;
  char name[NAME_LENGTH + 1];
  sprintf(name, "%.*s", NAME_LENGTH, name_token.start);

  if (lookup_token(context, name)) {
    error(parser, "Cannot redefine function.", SEMANTICERROR);
    advance(parser);
  }

  register_fxn(context, name, TYPE_NULL);
  function(parser, context, name);
}

void parse_function(Parser *parser, CodegenContext *context) {
  // consume(parser, TOKEN_FXN, "Expected function declaration keyword 'fxn'.");
  advance(parser);

  switch (parser->current.type) {
  case TOKEN_RUN:
  case TOKEN_IDENTIFIER: {
    register_and_form_fxn(parser, context);
    break;
  }
  default:
    error(parser, "Cannot use token to create fxn.", SYNTAXERROR);
    advance(parser);
  }
}

void begin(Parser *parser, CodegenContext *context) {
  while (parser->current.type == TOKEN_FXN) {
    parse_function(parser, context);
  }
}

void compile_parse(Lexer *lexer, errorStack *s) {
  Parser parser;
  parser.lexer = lexer;
  // Prime  by fetching the first token. That way parser.current is not NULL
  advance(&parser);

  // Startup the backend
  CodegenContext code_writer = {0};
  codegen_init(&code_writer, "output.ll");

  // Now parsing the function while sharing the backend context.
  begin(&parser, &code_writer);

  // Checks the end of the file.
  consume(
      &parser, TOKEN_EOF,
      "Unexpected trailing syntax tokens encountered after main entry block.");

  fclose(code_writer.file);
  free_codegen_context(&code_writer, s);

  printf("Checking errors before compilation...");
}

/*--------------------------------------------------------------------------------

                                    HELPERS

---------------------------------------------------------------------------------*/

/*Register Variable in Symbol Table*/
void register_variable(CodegenContext *context, const char *name,
                       datatype variable_type) {

  grow_symbols_if_needed(context);

  Symbol *sym = &context->symbols[context->symbol_count++];

  char *new_space = realloc_space(sym->name, strlen(name) + 1);
  sym->name = new_space;
  sym->llvm_name = new_space;
  strcpy(sym->name, name);
  sym->type = variable_type;
  // snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", name);

  (variable_type == TYPE_STRING)
      ? sym->str_length = context->string_constant_count++
      : 0;

  sym->t_type = VAR;
}

void register_fxn(CodegenContext *context, const char *name,
                  datatype return_type) {

  grow_symbols_if_needed(context);
  Symbol *sym = &context->symbols[context->symbol_count++];

  char *new_space = realloc_space(sym->name, strlen(name) + 1);
  sym->name = new_space;
  sym->llvm_name = new_space;
  strcpy(sym->name, name);
  sym->type = return_type;
  sym->str_length = 0;
  sym->t_type = FUNC;
}
