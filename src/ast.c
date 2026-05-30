#include <stdio.h>
#include <stdlib.h>
#include "../headers/ast.h"

static ASTNode* allocate_node(ASTNodeType type){
    ASTNode* node = malloc(sizeof(ASTNode));

    if(node == NULL){
        fprintf(stderr, "Could not allocate AST node.\n");
        exit(1);
    }

    node->Type = type;

    return node;
}

ASTNode* create_literal_node(Token token){
    ASTNode* node = allocate_node(AST_LITERAL_EXPR);
    node->literal_expr.token = token;

    return node;

}

ASTNode* create_println_node(ASTNode* value){
    ASTNode* node = allocate_node(AST_PRINTLN);

    node->println.value = value;

    return node;
}

ASTNode* create_binary_node(ASTNode* left, TokenType operator_type, ASTNode* right){
    ASTNode* node = allocate_node(AST_BINARY_EXPR);

    node->binary_expr.left = left;
    node->binary_expr.operator_type = operator_type;
    node->binary_expr.right = right;

    return node;
}

