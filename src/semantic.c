#include "../headers/semantic.h"
#include "../headers/functions.h"
#include "../headers/variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for internal tree-walking functions
static void analyze_node(SemanticContext *context, ASTNode *node);
static void analyze_block(SemanticContext *context, ASTNode *block_node);

// Helper to push semantic errors
static void report_semantic_error(SemanticContext *context, const char *msg) {
  Error *e = malloc(sizeof(Error));
  e->token = (Token){0};
  e->type = SEMANTICERROR;
  e->message = strdup(msg);
  e->got = strdup("semantic_analysis");
  e->line = 0;
  // Token could be left empty or filled if node has line info.
  errorStack_push(context->errors, e);
}

// Helper to infer expression type
static DataType infer_expr_type(SemanticContext *context, ASTNode *expr) {
  if (!expr)
    return TYPE_NULL;

  if (expr->Type == AST_LITERAL_EXPR) {
    if (expr->literal_expr.token.type == TOKEN_FLOAT)
      return TYPE_FLOAT;
    if (expr->literal_expr.token.type == TOKEN_BOOL)
      return TYPE_BOOL;
    if (expr->literal_expr.token.type == TOKEN_NULL)
      return TYPE_NULL;
    if (expr->literal_expr.token.type == TOKEN_STRING)
      return TYPE_STRING;
    return TYPE_INT;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    DataType left_type = infer_expr_type(context, expr->binary_expr.left);
    DataType right_type = infer_expr_type(context, expr->binary_expr.right);
    return (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT
                                                                 : TYPE_INT;
  }

  if (expr->Type == AST_CONCAT_STR) {
    return TYPE_STRING;
  }

  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char *var_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
    memcpy(var_name, expr->var_ref.name, NAME_LENGTH);
    var_name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn,
                               expr->var_ref.level);
    if (sym) {
      return sym->type;
    } else {
      report_semantic_error(context,
                            "Unrecognized variable referenced in expression.");
    }
  }

  if (expr->Type == AST_CALL_FXN) {
    const int NAME_LENGTH = expr->call_fxn.name_length;
    char *fn_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
    memcpy(fn_name, expr->call_fxn.name, NAME_LENGTH);
    fn_name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, fn_name, &expr->call_fxn.fxn,
                               expr->call_fxn.level);
    if (sym) {
      return sym->type;
    }
  }
  report_semantic_error(context, "Could not infer expression type.");
  return TYPE_NULL;
}

// Main entry point
void analyze_semantics(SemanticContext *context, ASTNode *node) {
  if (!node)
    return;

  // Typically the root is an AST_PROGRAM
  analyze_node(context, node);
}

// Recursive tree-walker
static void analyze_node(SemanticContext *context, ASTNode *node) {

  if (!node)
    return;

  switch (node->Type) {
  case AST_PROGRAM:
    if (node->program.function) {
      analyze_node(context, node->program.function);
    }
    break;

  case AST_FUNCTION:
    // Register function
    if (lookup_token(context->codegen, node->function.name, &node->function.fxn,
                     node->function.level)) {
      report_semantic_error(context, "Cannot redefine function.");
    } else {
      node->function.resolved_symbol = register_fxn(
          context->codegen, node->function.name, node->function.fxn.return_type,
          node->function.level, &node->function.fxn);
    }

    // Analyze Parameters.
    Params *param = node->function.fxn.params;
    while (param) {
      analyze_node(context, param->param);
      param = param->next;
    }

    // Analyze Function Body.
    if (node->function.body) {
      analyze_node(context, node->function.body);
    }

    break;

  case AST_BLOCK:
    analyze_block(context, node);
    break;

  case AST_VAR_DECL: {

    const int NAME_LENGTH = node->var_decl.name_length;
    char name[NAME_LENGTH + 1];
    // snprintf(name, NAME_LENGTH+1, "%.*s", NAME_LENGTH, node->var_decl.name);
    memcpy(name, node->var_decl.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, name, &node->var_decl.fxn,
                               node->var_decl.level);
    if (sym) {
      report_semantic_error(context, "Multiple definition of variable.");
    }

    DataType inferred;
    if (node->var_decl.value) {
      analyze_node(context, node->var_decl.value);
      if (node->var_decl.value_type == TYPE_NULL) {
        inferred = infer_expr_type(context, node->var_decl.value);
        node->var_decl.value_type = inferred;

      } else {
        DataType expected_type = node->var_decl.value_type;
        inferred = infer_expr_type(context, node->var_decl.value);
        if (inferred != expected_type) {
          report_semantic_error(context, "Datatype Mismatch.");
        }
      }

      node->var_decl.resolved_symbol = register_variable(
          context->codegen, name, inferred, &node->var_decl.fxn, node->var_decl.level);

    } else {
      node->var_decl.value_type = TYPE_NULL;
      node->var_decl.resolved_symbol = register_variable(
          context->codegen, name, TYPE_NULL, &node->var_decl.fxn, node->var_decl.level);
    }
    break;
  }

  case AST_VAR_ASS: {
    const int NAME_LENGTH = node->var_assign.name_length;
    char name[NAME_LENGTH + 1];
    // snprintf(name, NAME_LENGTH + 1, "%.*s", NAME_LENGTH,
    // node->var_assign.name);

    memcpy(name, node->var_assign.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_assign.fxn,
                               node->var_assign.level);
    if (!sym) {
      report_semantic_error(context, "Assignment to undeclared variable.");
    } else {
      node->var_assign.resolved_symbol = sym;
      if (node->var_assign.value) {
        analyze_node(context, node->var_assign.value);
        DataType inferred = infer_expr_type(context, node->var_assign.value);

        if (sym->type != TYPE_NULL && inferred != TYPE_NULL &&
            sym->type != inferred) {
          report_semantic_error(context, "Incompatible assignment type.");
        } else if (sym->type == TYPE_NULL && inferred != TYPE_NULL) {
          sym->type = inferred; // update symbol type if it was null
        }
      }
    }
    break;
  }

  /*case AST_PRINTLN:
    if (node->println.value) {
      analyze_node(context, node->println.value);
    }
    break;*/

  case AST_IF:
    analyze_node(context, node->if_stmt.condition);
    analyze_node(context, node->if_stmt.then_block);
    if (node->if_stmt.else_block) {
      analyze_node(context, node->if_stmt.else_block);
    }
    break;

  case AST_WHILE:
    analyze_node(context, node->while_lp.condition);
    analyze_node(context, node->while_lp.then_block);
    break;

  case AST_FOR:
    analyze_node(context, node->for_lp.variable);
    analyze_node(context, node->for_lp.condtion);
    analyze_node(context, node->for_lp.var_operation);
    analyze_node(context, node->for_lp.then_block);
    break;

  case AST_BINARY_EXPR:
    analyze_node(context, node->binary_expr.left);
    analyze_node(context, node->binary_expr.right);
    break;

  case AST_CONCAT_STR:
    analyze_node(context, node->str_concat.left);
    analyze_node(context, node->str_concat.right);
    break;

  case AST_RET_NODE:
    if (node->ret_node.value) {
      analyze_node(context, node->ret_node.value);
    }
    break;

  case AST_CALL_FXN: {
    const int NAME_LENGTH = node->call_fxn.name_length;
    char fn_name[NAME_LENGTH + 1];
    memcpy(fn_name, node->call_fxn.name, NAME_LENGTH);
    fn_name[NAME_LENGTH] = '\0';

    printf("FN NAME: %s\n", fn_name);
    bool is_println = (memcmp(fn_name, "println", 7) == 0);
    Symbol *sym = NULL;

    if (is_println) {
      node->call_fxn.return_type = TYPE_NULL;
    } else {
      sym = lookup_token(context->codegen, fn_name, &node->call_fxn.fxn,
                         node->call_fxn.level);
      if (!sym) {
        report_semantic_error(context, "Call to undeclared function.");
        break;
      }
      node->call_fxn.return_type = sym->type;
      node->call_fxn.resolved_symbol = sym;
    }

    Params *expected_param = (sym && sym->fxn) ? sym->fxn->params : NULL;
    Args *passed_arg = node->call_fxn.args;

    while (passed_arg != NULL) {
      analyze_node(context, passed_arg->arg);
      passed_arg->datatype = infer_expr_type(context, passed_arg->arg);

      if (expected_param != NULL) {
        if (passed_arg->datatype != expected_param->param->var_decl.value_type) {
          report_semantic_error(context,
                                "Argument type mismatch in function call.");
        }
        expected_param = expected_param->next;
      }

      passed_arg = passed_arg->next;
    }

    break;
  }

  case AST_LITERAL_EXPR:
    break;

  case AST_VAR_REF: {
    const int NAME_LENGTH = node->var_ref.name_length;
    char name[NAME_LENGTH + 1];
    // snprintf(name, NAME_LENGTH + 1, "%.*s", NAME_LENGTH, node->var_ref.name);
    memcpy(name, node->var_ref.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_ref.fxn,
                               node->var_ref.level);
    if (!sym) {
      report_semantic_error(
          context, "Referenced Variable not found in the current scope.");
    } else {
      node->var_ref.resolved_symbol = sym;
    }
    break;
  }
  default:
    break;
  }
}

// Walks through all statements in a block
static void analyze_block(SemanticContext *context, ASTNode *block_node) {
  if (!block_node || block_node->Type != AST_BLOCK)
    return;

  int initial_symbol_count = context->codegen->symbol_count;

  for (int i = 0; i < block_node->block.count; ++i) {
    analyze_node(context, block_node->block.statements[i]);
  }

  for (int i = initial_symbol_count; i < context->codegen->symbol_count; ++i) {
    context->codegen->symbols[i].is_active = false;
  }
}
