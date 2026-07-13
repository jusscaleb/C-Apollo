#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "defs.h"
#include "error.h"
#include "token.h"

// The semantic context might eventually need its own symbol table,
// but for now we just walk the tree.

typedef struct {
  ErrorStack *errors;
  CodegenContext *codegen;
} SemanticContext;

/**
 * Main entry point for semantic analysis.
 * Performs type checking, scope validation, and name resolution on the AST.
 * @param context The semantic context containing the symbol table and error
 * stack.
 * @param node The root AST node to analyze.
 */
void analyze_semantics(SemanticContext *context, ASTNode *node);

#endif // SEMANTIC_H
