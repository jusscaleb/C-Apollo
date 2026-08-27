#include "../../headers/semantic.h"
#include <stdio.h>

// Helper to push semantic errors
void report_semantic_error(SemanticContext *context, const char *msg) {
  if (!context) return;
  Error *e = malloc(sizeof(Error));
  e->token = (Token){0};
  e->type = SEMANTICERROR;
  e->message = strdup(msg);
  e->got = strdup("semantic_analysis");
  e->line = 0;
  errorStack_push(context->errors, e);
}

DataType infer_expr_type(SemanticContext *context, ASTNode *expr) {

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
    if (expr->literal_expr.token.type == TOKEN_INT)
      return TYPE_INT;
    if (expr->literal_expr.token.type == TOKEN_CHAR)
      return TYPE_CHAR;
    return TYPE_NULL;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    switch (expr->binary_expr.operator_type) {
    case TOKEN_GT:
    case TOKEN_ST:
    case TOKEN_SE:
    case TOKEN_GE:
    case TOKEN_EQT:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_NEQ:
      return TYPE_BOOL;
    default: {
      DataType left_type = infer_expr_type(context, expr->binary_expr.left);
      DataType right_type = infer_expr_type(context, expr->binary_expr.right);
      
      if (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT)
        return TYPE_FLOAT;
      if (left_type == TYPE_CHAR || right_type == TYPE_CHAR)
        return TYPE_CHAR;

      return TYPE_INT;
    }
    }
  }
  if (expr->Type == AST_URINARY_EXPR) {
    switch (expr->urinary_expr.operator_type) {
    case TOKEN_REF:
    case TOKEN_MUL:
    case TOKEN_SUB:
      return infer_expr_type(context, expr->urinary_expr.value);
    default:
      report_semantic_error(context, "Unrecognized operator type.");
      return TYPE_NULL;
    }
  }



  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[NAME_LENGTH + 1];
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
    char fn_name[NAME_LENGTH + 1];
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

int infer_expr_pointer_level(SemanticContext *context, ASTNode *expr) {
  if (!expr)
    return 0;

  if (expr->Type == AST_LITERAL_EXPR || expr->Type == AST_BINARY_EXPR) {
    return 0;
  }

  if (expr->Type == AST_URINARY_EXPR) {
    switch (expr->urinary_expr.operator_type) {
    case TOKEN_REF:
      return infer_expr_pointer_level(context, expr->urinary_expr.value) + 1;
    case TOKEN_MUL: {
      int inner_level = infer_expr_pointer_level(context, expr->urinary_expr.value);
      if (inner_level <= 0) {
        report_semantic_error(context, "Cannot dereference non-pointer type.");
        return 0;
      }
      return inner_level - 1;
    }
    case TOKEN_SUB:
      return infer_expr_pointer_level(context, expr->urinary_expr.value);
    default:
      return 0;
    }
  }

  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[NAME_LENGTH + 1];
    memcpy(var_name, expr->var_ref.name, NAME_LENGTH);
    var_name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn,
                               expr->var_ref.level);
    if (sym) {
      return sym->pointer_level;
    }
  }

  if (expr->Type == AST_CALL_FXN) {
    const int NAME_LENGTH = expr->call_fxn.name_length;
    char fn_name[NAME_LENGTH + 1];
    memcpy(fn_name, expr->call_fxn.name, NAME_LENGTH);
    fn_name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, fn_name, &expr->call_fxn.fxn,
                               expr->call_fxn.level);
    if (sym) {
      return sym->pointer_level;
    }
  }

  return 0;
}


MemoryBucket infer_bucket_type(SemanticContext *context, ASTNode *expr){
  if(!expr)
    return 0;
  switch(expr->Type){
    case AST_BINARY_EXPR: {
      MemoryBucket right_b = infer_bucket_type(context, expr->binary_expr.right);
      MemoryBucket left_b = infer_bucket_type(context, expr->binary_expr.left);

      if (right_b == BUCKET_ONE) right_b = BUCKET_PLUS_ONE;
      if (left_b == BUCKET_ONE) left_b = BUCKET_PLUS_ONE;

      if (left_b == BUCKET_THREE || right_b == BUCKET_THREE) return BUCKET_THREE;
      if (left_b == BUCKET_TWO || right_b == BUCKET_TWO) return BUCKET_TWO;
      return BUCKET_PLUS_ONE;
    }
    case AST_URINARY_EXPR:
      return infer_bucket_type(context, expr->urinary_expr.value);
    case AST_CALL_FXN:
      if (expr->call_fxn.resolved_symbol) return expr->call_fxn.resolved_symbol->bucket;
      return BUCKET_TWO;
    case AST_VAR_REF:
      if (expr->var_ref.resolved_symbol) return expr->var_ref.resolved_symbol->bucket;
      return BUCKET_PLUS_ONE;
    default: return BUCKET_PLUS_ONE;
  }
  return BUCKET_PLUS_ONE;
}