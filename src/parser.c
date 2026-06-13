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

  if (token.type == TOKEN_FLOAT) {
    consume(parser, TOKEN_FLOAT, "Expected float literal.");
    return create_literal_node(token);
  }

  if (token.type == TOKEN_IDENTIFIER) {
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
    return create_var_ref_node(token.start, token.length);
  }

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
  advance(parser);
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' for arguments.");

  switch (parser->current.type) {
  case TOKEN_STRING:
  case TOKEN_BOOL: {
    Token literal_token = parser->current;
    advance(parser);

    ASTNode *literal = create_literal_node(literal_token);
    ASTNode *println_node = create_println_node(literal);
    gen_println_from_ast(context, println_node);
    break;
  }

  case TOKEN_INT:
  case TOKEN_FLOAT:
  case TOKEN_IDENTIFIER: {
    ASTNode *expr = parse_expression(parser);
    ASTNode *println_node = create_println_node(expr);
    gen_println_from_ast(context, println_node);
    break;
}


  default: {
    char var_name[64];
    sprintf(var_name, "%.*s", parser->current.length, parser->current.start);
    Symbol *sym = lookup_variable(context, var_name);

    if (!sym) {
      fprintf(stderr,
              "Apollo Syntax Error [Line %d]: Expected println argument. \nGot "
              "'%.*s'\n",
              parser->current.line, parser->current.length,
              parser->current.start);
      exit(1);
    }
      ASTNode *var_ref = create_var_ref_node(parser->current.start, parser->current.length);
      ASTNode *println_node = create_println_node(var_ref);
      gen_println_from_ast(context, println_node);
      advance(parser);

      return;
      


  }
  }

  consume(parser, TOKEN_RPARETH,
          "Expected closing parenthesis ')' for arguments.");
  consume(parser, TOKEN_SEMICOLON,
          "Expected trailing semicolon ';' to terminate statement.");
}

void var(Parser *parser, CodegenContext *context) {
  advance(parser);

  Token name_token = parser->current;
  char name[64];
  sprintf(name, "%.*s", name_token.length, name_token.start);

  consume(parser, TOKEN_IDENTIFIER, "Expected identifier for variable.");
  consume(parser, TOKEN_ASSIGN, "Expected '=' after identifier.");

  Variable variable;
  ASTNode *value;

  switch (parser->current.type) {
  case TOKEN_INT:
  case TOKEN_FLOAT:
  case TOKEN_IDENTIFIER: {
    value = parse_expression(parser);
    // Type registration is deferred to gen_var_decl_from_ast
    // which infers the type from the ExprResult at codegen time
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

  case TOKEN_BOOL: {
    Token value_token = parser->current;
    variable = (Variable){TYPE_BOOL, parser->current.start,
                          parser->current.length, parser->current.line};
    consume(parser, TOKEN_BOOL,
            "Expected boolean literal for variable assignment.");
    value = create_literal_node(value_token);
    register_variable(context, name, variable.type);
    break;
  }

  default: {
    fprintf(stderr, "Apollo Syntax error [Line %d], expected variable value",
            parser->current.line);
    exit(1);
  }
  }

  consume(parser, TOKEN_SEMICOLON, "Expected ';' to end line.");

  ASTNode *var_node = create_var_decl_node(name_token.start, name_token.length,
                                           variable.type, value);

  gen_var_decl_from_ast(context, var_node);
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

  default:
    fprintf(stderr, "Unrecognized token '%c'. [Line %d]", parser->current.type, parser->current.line);
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

// parse_function check if the function is grammatically correct. (fxn
// run()->(void){})
void parse_function(Parser *parser, CodegenContext *context) {
  // verifies fxn run()
  consume(parser, TOKEN_FXN, "Expected function declaration keyword 'fxn'.");
  consume(parser, TOKEN_RUN, "Expected program entrypoint name 'run'.");
  consume(parser, TOKEN_LPARETH,
          "Expected parameter list wrapper starting with '('.");
  consume(parser, TOKEN_RPARETH,
          "Expected closing parameter list wrapper ')'.");

  // verifies the return type...
  consume(parser, TOKEN_ARROW, "Expected return signature pointer token '->'.");
  consume(parser, TOKEN_LPARETH,
          "Expected open parenthesis '(' around return type specification.");
  consume(parser, TOKEN_VOID,
          "Expected explicit type parameter keyword 'void'.");
  consume(parser, TOKEN_RPARETH,
          "Expected closing parenthesis ')' around return type specification.");

  // The full function signature is valid, so the backend can open the LLVM
  // function.
  gen_function_start(context, "run");

  // Parse each statement in the body and emit its matching backend code.
  parse_block(parser, context);

  // Close the generated main function after the source block has been parsed.
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