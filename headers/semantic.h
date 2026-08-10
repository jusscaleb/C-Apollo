/*===================================================================
                          semantic.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Semantics Compile Time library.
      Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "../headers/functions.h"
#include "../headers/variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

DataType infer_expr_type(SemanticContext *context, ASTNode *expr);
void report_semantic_error(SemanticContext *context, const char *msg);

// Forward declarations for internal tree-walking functions
static void analyze_node(SemanticContext *context, ASTNode *node);
static void analyze_block(SemanticContext *context, ASTNode *block_node);


#endif
