/*--------------------------------------------------------------------------------

                          THE NEAT FREAK :)

---------------------------------------------------------------------------------*/

#include "../headers/ast.h"
#include <stdio.h>
#include <stdlib.h>

static ASTNode *allocate_node(ASTNodeType type) {
  ASTNode *node = malloc(sizeof(ASTNode));

  if (node == NULL) {
    fprintf(stderr, "Could not allocate AST node.\n");
    exit(EXIT_FAILURE);
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

ASTNode *create_var_ref_node(const char *name, int name_length) {
  ASTNode *node = allocate_node(AST_VAR_REF);
  node->var_ref.name = name;
  node->var_ref.name_length = name_length;
  return node;
}

ASTNode *create_var_assign_node(const char *name, int name_length,
                                ASTNode *value) {
  ASTNode *node = allocate_node(AST_VAR_ASS);
  node->var_assign.name = name;
  node->var_assign.name_length = name_length;
  node->var_assign.value = value;
  return node;
}

ASTNode *create_concat_node(ASTNode *left, ASTNode *right) {
  ASTNode *node = allocate_node(AST_CONCAT_STR);
  node->str_concat.left = left;
  node->str_concat.right = right;

  return node;
}

ASTNode *create_if_node(ASTNode *condition, ASTNode *then_block,
                        ASTNode *else_block) {
  ASTNode *node = allocate_node(AST_IF);
  node->if_stmt.condition = condition;
  node->if_stmt.then_block = then_block;
  node->if_stmt.else_block = else_block;
  return node;
}

ASTNode *create_while_node(ASTNode *condition, ASTNode *then_block) {
  ASTNode *node = allocate_node(AST_WHILE);
  node->while_lp.condition = condition;
  node->while_lp.then_block = then_block;

  return node;
}

ASTNode *create_block_node(void) {
  ASTNode *node = allocate_node(AST_BLOCK);
  node->block.count = 0;
  node->block.capacity = 8;
  node->block.statements = malloc(sizeof(ASTNode *) * node->block.capacity);
  return node;
}

ASTNode *create_for_node(ASTNode *variable, ASTNode *condition,
                         ASTNode *var_operation, ASTNode* then_block) {
  ASTNode *node = allocate_node(AST_FOR);
  node->for_lp.var_operation = var_operation;
  node->for_lp.condtion = condition;
  node->for_lp.variable = variable;
  node->for_lp.then_block = then_block;
  return node;
}

void block_add_statement(ASTNode *block, ASTNode *statement) {
  if (block->block.count >= block->block.capacity) {
    block->block.capacity *= 2;
    block->block.statements = realloc(
        block->block.statements, sizeof(ASTNode *) * block->block.capacity);
  }
  block->block.statements[block->block.count++] = statement;
}
