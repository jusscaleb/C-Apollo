#include "../../headers/semantic.h"

// Helper to push semantic errors
void report_semantic_error(SemanticContext *context, const char *msg) {
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
    if(expr->literal_expr.token.type == TOKEN_INT)
      return TYPE_INT;
    if(expr->literal_expr.token.type == TOKEN_CHAR)
      return TYPE_CHAR;
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
        if (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) return TYPE_FLOAT;
        if (left_type == TYPE_CHAR || right_type == TYPE_CHAR) return TYPE_CHAR;
        
        return TYPE_INT;
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

