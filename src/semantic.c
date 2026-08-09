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
  
    if(expr->literal_expr.token.type == TOKEN_INT)
      return TYPE_INT;
    return TYPE_NULL;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    switch(expr->binary_expr.operator_type){
      case TOKEN_GT:
      case TOKEN_ST:
      case TOKEN_SE:
      case TOKEN_GE:
      case TOKEN_EQT:
      case TOKEN_AND:
      case TOKEN_OR:
      case TOKEN_NEQ:
        return TYPE_BOOL;
      default:{
        DataType left_type = infer_expr_type(context, expr->binary_expr.left);
        DataType right_type = infer_expr_type(context, expr->binary_expr.right);
        return (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT
                                                                    : TYPE_INT;
      }


    }
  }

  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[NAME_LENGTH+1];
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
    char fn_name[NAME_LENGTH+1];
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

  analyze_node(context, node);
}

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
    if (lookup_token(context->codegen, node->function.name, &node->function.fxn,
                     node->function.level)) {
      report_semantic_error(context, "Cannot redefine function.");
      break;
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
    memcpy(name, node->var_decl.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, name, &node->var_decl.fxn,
                               node->var_decl.level);
    if (sym) {
      report_semantic_error(context, "Multiple definition of variable.");
      break;
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
          break;
        }
      }

      node->var_decl.resolved_symbol =
          register_variable(context->codegen, name, inferred,
                            &node->var_decl.fxn, node->var_decl.level);

    } else {
      node->var_decl.value_type = TYPE_NULL;
      node->var_decl.resolved_symbol =
          register_variable(context->codegen, name, TYPE_NULL,
                            &node->var_decl.fxn, node->var_decl.level);
    }
    break;
  }

  case AST_VAR_ASS: {
    const int NAME_LENGTH = node->var_assign.name_length;
    char name[NAME_LENGTH + 1];
    memcpy(name, node->var_assign.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_assign.fxn,
                               node->var_assign.level);
    if (!sym) {
      report_semantic_error(context, "Assignment to undeclared variable.");
      break;
    } else {
      node->var_assign.resolved_symbol = sym;
      if (node->var_assign.value) {
        analyze_node(context, node->var_assign.value);
        DataType inferred = infer_expr_type(context, node->var_assign.value);

        if (sym->type != TYPE_NULL && inferred != TYPE_NULL &&
            sym->type != inferred) {
          report_semantic_error(context, "Incompatible assignment type.");
          break;
        } else if (sym->type == TYPE_NULL && inferred != TYPE_NULL) {
          sym->type = inferred; 
        }
      }
    }
    break;
  }

  case AST_IF:{
    ASTNode *cond = node->if_stmt.condition;
    if(infer_expr_type(context, cond) != TYPE_BOOL){
      report_semantic_error(context, "Expected a boolean expression or literal.");
      break;
    }
    analyze_node(context, node->if_stmt.condition);
    analyze_node(context, node->if_stmt.then_block);
    if (node->if_stmt.else_block) {
      analyze_node(context, node->if_stmt.else_block);
    }
    break;

  }
  case AST_WHILE:
  {
  ASTNode *cond = node->while_lp.condition;
  if(infer_expr_type(context, cond) != TYPE_BOOL){
      report_semantic_error(context, "Expected a boolean expression or literal.");
      break;
    }
    analyze_node(context, node->while_lp.condition);
    analyze_node(context, node->while_lp.then_block);
    break;

  }
  case AST_FOR:
    ASTNode *cond = node->for_lp.condtion;
    ASTNode *op = node->for_lp.var_operation;

    bool case_1 = cond->Type != AST_BINARY_EXPR;
    bool case_2 = infer_expr_type(context, cond) != TYPE_BOOL;

    if(case_1 || case_2){
      report_semantic_error(context, "Expected a boolean binary expression: E.g.: 'x > 5'");
      break;
    }
    
    analyze_node(context, node->for_lp.variable);
    analyze_node(context, node->for_lp.condtion);
    analyze_node(context, node->for_lp.var_operation);
    analyze_node(context, node->for_lp.then_block);
    break;

  case AST_BINARY_EXPR:
    ASTNode *left = node->binary_expr.left;
    ASTNode *right = node->binary_expr.right;

    DataType l_type = infer_expr_type(context, left),
    r_type = infer_expr_type(context, right);

    char msg[16];
    if (l_type != r_type) {    
      if(l_type == TYPE_INT && r_type != TYPE_FLOAT 
        || (l_type == TYPE_BOOL || r_type == TYPE_BOOL)
      ){
        report_semantic_error(context, "Binary expression contains incompatible operands.");
        break;
      }
    }
    analyze_node(context, left);
    analyze_node(context, right);
    break;

  case AST_RET_NODE:
    if (node->ret_node.value) {
      if(node->ret_node.fxn.return_type == TYPE_NULL){
        report_semantic_error(context, "Fxn of return type \"null\" cannot return value.");
        break;
      }
      analyze_node(context, node->ret_node.value);
    }else{
      report_semantic_error(context, "Value not provided.");
      break;
    }
    break;

  case AST_CALL_FXN: {
    const int NAME_LENGTH = node->call_fxn.name_length;
    char fn_name[NAME_LENGTH + 1];
    memcpy(fn_name, node->call_fxn.name, NAME_LENGTH);
    fn_name[NAME_LENGTH] = '\0';

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
        if (passed_arg->datatype !=
            expected_param->param->var_decl.value_type) {
          report_semantic_error(context, "Argument type mismatch in function call.");
          break;
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
    memcpy(name, node->var_ref.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_ref.fxn,
                               node->var_ref.level);
    if (!sym) {
      report_semantic_error(
          context, "Referenced Variable not found in the current scope.");
          break;
    } else {
      node->var_ref.resolved_symbol = sym;
    }
    break;
  }
  default:
    break;
  }
}

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
