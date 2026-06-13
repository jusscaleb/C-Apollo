/*--------------------------------------------------------------------------------

                        TRAFFIC POLICE

---------------------------------------------------------------------------------*/

#include "../headers/ast.h"
#include "../headers/defs.h"
#include "../headers/token.h"
#include "../headers/variables.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// move on to next token
static void advance(Parser *parser) {
  parser->previous = parser->current;
  parser->current = next_token(parser->lexer);
}

// checks if given token matches expected token.
static void consume(Parser *parser, TokenType type, const char *errorMessage) {

  if (parser->current.type == type) {
    advance(parser);
    return;
  }

  fprintf(stderr, "Apollo Syntax Error [Line %d]: %s\n", parser->current.line,
          errorMessage);
  fprintf(stderr, "Found: '%.*s' (Type: %s)\n", parser->current.length,
          parser->current.start, TokenNames[parser->current.type]);

  exit(1);
}

/*------------------ARITHMETICS------------------*/

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

  /*if(token.type == TOKEN_SEMICOLON){
    token.type = TOKEN_NULL;
    return create_var_ref_node(token.start, token.length);
  }*/

  fprintf(stderr, "Apollo Syntax Error [Line %d]: Expected expression value.\n",
          parser->current.line);
  exit(1);
}

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

static ASTNode *parse_comparison(Parser *parser) {
  ASTNode *left = parse_expression(parser);

  while (parser->current.type == TOKEN_GT  ||
         parser->current.type == TOKEN_ST  ||
         parser->current.type == TOKEN_GE  ||
         parser->current.type == TOKEN_SE  ||
         parser->current.type == TOKEN_EQT ||
         parser->current.type == TOKEN_NEQ) {
    TokenType operator_type = parser->current.type;
    advance(parser);

    ASTNode *right = parse_expression(parser);
    left = create_binary_node(left, operator_type, right);
  }

  return left;
}

static datatype infer_expr_type(ASTNode *expr) {
  if (expr->Type == AST_LITERAL_EXPR) {
    return expr->literal_expr.token.type == TOKEN_FLOAT ? TYPE_FLOAT : TYPE_INT;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    datatype left_type = infer_expr_type(expr->binary_expr.left);
    datatype right_type = infer_expr_type(expr->binary_expr.right);

    return (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT
                                                                 : TYPE_INT;
  }

  fprintf(stderr, "Apollo Syntax Error: Could not infer expression type.\n");
  exit(1);
}

static void println(Parser *parser, CodegenContext *context) {
  advance(parser); // Move past TOKEN_PRINTLN
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' for arguments.");

  // Parse whatever is inside the parentheses as a unified expression
  ASTNode *expr = parse_comparison(parser);

  ASTNode *println_node = create_println_node(expr);
  gen_println_from_ast(context, println_node);

  consume(parser, TOKEN_RPARETH,
          "Expected close parenthesis ')' after arguments.");
  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
}

void var(Parser *parser, CodegenContext *context) {
  advance(parser);

  set declaring = false;

  Token name_token = parser->current;
  char name[64];
  sprintf(name, "%.*s", name_token.length, name_token.start);

  Symbol* sym = lookup_variable(context, name);

  if(sym){
    fprintf(stderr, "Apollo  Error [Line %d], Multiple definition of variable '%s'",
            parser->current.line, sym->llvm_name);
    exit(1);
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

  if(!declaring)
    consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");

  switch (parser->current.type) {

  case TOKEN_INT:
  case TOKEN_FLOAT:
  case TOKEN_IDENTIFIER: {
    value = parse_comparison(parser);
    variable = (Variable){TYPE_INT, NULL, 0, parser->current.line};
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

    if(!declaring){
    fprintf(stderr, "Apollo Syntax error [Line %d], expected variable value",
            parser->current.line);
    exit(1);

    }
  }
  }

  

  ASTNode *var_node = create_var_decl_node(name_token.start, name_token.length,
                                           variable.type, value);

  gen_var_decl_from_ast(context, var_node);
  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");
}

void identifier(Parser *parser, CodegenContext *context) {
  ASTNode *value;

  char potential_var_name[64];
  sprintf(potential_var_name, "%.*s", parser->current.length,
          parser->current.start);
  Symbol *sym = lookup_variable(context, potential_var_name);

  if (!sym) {
    fprintf(stderr, "Unrecognized token. [Line %d]", parser->current.line);
    exit(1);
  }

  advance(parser);
  consume(parser, TOKEN_ASSIGN, "Expected '=' after variable.");

  Token t = parser->current;
  switch (parser->current.type) {
  case TOKEN_STRING: {
    if (sym->type != TYPE_STRING && sym->type != TYPE_NULL) {
      fprintf(stderr, "Incompatible assignment type. [Line %d] Got type %c",
              parser->current.line, sym->type);
      exit(1);
    }
    value = create_literal_node(t);
    advance(parser);
    break;
  }

  case TOKEN_INT: {
    if (sym->type != TYPE_INT && sym->type != TYPE_NULL) {
      fprintf(stderr, "Incompatible assignment type. [Line %d]",
              parser->current.line);
      exit(1);
    }
    value = parse_comparison(parser);
    break;
  }

  case TOKEN_FLOAT: {
    if (sym->type != TYPE_FLOAT && sym->type != TYPE_NULL) {
      fprintf(stderr, "Incompatible assignment type. [Line %d]",
              parser->current.line);
      exit(1);
    }
    value = parse_comparison(parser);
    break;
  }

  case TOKEN_BOOL: {
    if (sym->type != TYPE_BOOL && sym->type != TYPE_NULL) {
      fprintf(stderr, "Incompatible assignment type. [Line %d]. Found type %c",
              parser->current.line, sym->type);
      exit(1);
    }
    value = create_literal_node(t);
    advance(parser);
    break;
  }

  case TOKEN_IDENTIFIER: {
    char id_name[64];
    sprintf(id_name, "%.*s", parser->current.length, parser->current.start);
    Symbol *var = lookup_variable(context, id_name);

    if (!var) {
      fprintf(stderr, "Unrecognized token. [Line %d]", parser->current.line);
      exit(1);
    }

    if (var->type != sym->type && sym->type != TYPE_NULL) {
      fprintf(stderr, "Incompatible assignment type. [Line %d]",
              parser->current.line);
      exit(1);
    }

    value = parse_comparison(parser);

    break;
  }

  default: {
    fprintf(stderr, "Invalid assignment type. [Line %d]", parser->current.line);
    exit(1);
  }
  }

  ASTNode *assign_var =
      create_var_assign_node(sym->llvm_name, strlen(sym->llvm_name), value);
  gen_var_assign_from_ast(context, assign_var);
  consume(parser, TOKEN_SEMICOLON,
          "Expected trailing semicolon ';' to terminate statement.");
}

// check the body statement
static void parse_body_statement(Parser *parser, CodegenContext *context) {
  switch (parser->current.type) {

  case TOKEN_PRINTLN:
    println(parser, context);
    break;

  case TOKEN_VAR:
    var(parser, context);
    break;

  case TOKEN_NULL:
  case TOKEN_IDENTIFIER:
    identifier(parser, context);
    break;

  default:
    fprintf(stderr, "Unrecognized token. [Line %d]", parser->current.type);
    exit(1);
  }
}

// parse_block() checks the innard of that function.
static void parse_block(Parser *parser, CodegenContext *context) {
  consume(parser, TOKEN_LBRACE,
          "Expected open brace '{' to begin function block definition.");

  while (parser->current.type != TOKEN_RBRACE &&
         parser->current.type != TOKEN_EOF) {
    parse_body_statement(parser, context);
  }

  consume(parser, TOKEN_RBRACE,
          "Expected closing brace '}' to terminate block context.");
}


void parse_function(Parser *parser, CodegenContext *context) {
  consume(parser, TOKEN_FXN, "Expected function declaration keyword 'fxn'.");
  consume(parser, TOKEN_RUN, "Expected program entrypoint name 'run'.");
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


  gen_function_start(context, "run");

  parse_block(parser, context);

  gen_function_end(context, true);
}

void compile_parse(Lexer *lexer) {
  Parser parser;
  parser.lexer = lexer;
  // Prime  by fetching the first token. That way parser.current is not NULL
  advance(&parser);

  // Startup the backend
  CodegenContext code_writer;
  codegen_init(&code_writer, "output.ll");

  // Now parsing the function while sharing the backend context.
  parse_function(&parser, &code_writer);

  // Checks the end of the file.
  consume(
      &parser, TOKEN_EOF,
      "Unexpected trailing syntax tokens encountered after main entry block.");

  printf("SUCCESSFUL.\n");
}

/*--------------------------------------------------------------------------------

                                    HELPERS

---------------------------------------------------------------------------------*/

/*Register Variable in Symbol Table*/
void register_variable(CodegenContext *context, const char *name,
                       datatype variable_type) {
  Symbol *sym = &context->symbols[context->symbol_count++];
  strncpy(sym->name, name, sizeof(sym->name));
  sym->type = variable_type;
  snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", name);

  (variable_type == TYPE_STRING)
      ? sym->str_length = context->string_constant_count++
      : 0;
}