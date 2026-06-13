/*--------------------------------------------------------------------------------

                       ABSTACT SYNTAX TREE :)

---------------------------------------------------------------------------------*/

#ifndef AST_H
#define AST_H
#include "arithmetic.h"
#include "token.h"
#include "variables.h"
#include <stdio.h>

// Kind of nodes to expect
typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_BLOCK,

  AST_VAR_DECL,
  AST_PRINTLN,

  AST_LITERAL_EXPR,
  AST_BINARY_EXPR,

  AST_VAR_REF
} ASTNodeType;

typedef struct ASTNode ASTNode;

// Stores the specified Node in the AST.
struct ASTNode {
  ASTNodeType Type;

  union {
    struct {
      ASTNode *function;

    } program;

    struct {
      const char *name;
      int name_length;
      ASTNode *body;

    } function;

    struct {
      ASTNode **statements;
      int count;
      int capacity;

    } block;

    struct {
      const char *name;
      int name_length;
      datatype value_type;
      ASTNode *value;

    } var_decl;

    struct {
      ASTNode *value;

    } println;

    struct {
      Token token;

    } literal_expr;

    struct {
      ASTNode *left;
      TokenType operator_type;
      ASTNode *right;
    } binary_expr;

  
    struct {
      const char *name;
      int name_length;
    }var_ref;
  };
};

ASTNode *create_literal_node(Token token);
ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right);
ASTNode *create_var_decl_node(const char *name, int name_length,
                              datatype value_type, ASTNode *value);
ASTNode *create_println_node(ASTNode *value);
ASTNode *create_block_node(void);

ASTNode *create_var_ref_node(const char *name, int name_length);



void block_add_statement(ASTNode *block, ASTNode *statement);

void gen_println_from_ast(CodegenContext *context, ASTNode *println_node);
ExprResult gen_expr_from_ast(CodegenContext *context, ASTNode *expr);

void gen_var_decl_from_ast(CodegenContext *context, ASTNode *var_node);




#endif
