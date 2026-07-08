#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "defs.h"
#include "error.h"

// The semantic context might eventually need its own symbol table,
// but for now we just walk the tree.
typedef struct {
  errorStack *errors;
  CodegenContext *codegen;
} SemanticContext;

// Main entry point for semantic analysis
void analyze_semantics(SemanticContext *context, ASTNode *node);

#endif // SEMANTIC_H
