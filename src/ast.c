#include "../headers/ast.h"
#include <stdio.h>
#include <stdlib.h>


static ASTNode *allocate_node(ASTNodeType type) {
  ASTNode *node = malloc(sizeof(ASTNode));

  if (node == NULL) {
    fprintf(stderr, "Could not allocate AST node.\n");
    exit(1);
  }

  node->Type = type;

  return node;
}

ASTNode *create_literal_node(Token token) {
  ASTNode *node = allocate_node(AST_LITERAL_EXPR);
  node->literal_expr.token = token;

  return node;
}

ASTNode *create_println_node(ASTNode *value) {
  ASTNode *node = allocate_node(AST_PRINTLN);

  node->println.value = value;

  return node;
}

ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right) {
  ASTNode *node = allocate_node(AST_BINARY_EXPR);

  node->binary_expr.left = left;
  node->binary_expr.operator_type = operator_type;
  node->binary_expr.right = right;

  return node;
}

ASTNode *create_var_decl_node(const char *name, int name_length,
                              datatype value_type, ASTNode *value) {
  ASTNode *node = allocate_node(AST_VAR_DECL);

  node->var_decl.name = name;
  node->var_decl.name_length = name_length;
  node->var_decl.value_type = value_type;
  node->var_decl.value = value;

  return node;
}
