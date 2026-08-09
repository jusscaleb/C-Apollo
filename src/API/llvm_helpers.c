#include "../../headers/llvm_backend.h"

LLVMValueRef load_variable(LLVMComponents *components, ASTNode *var_ref_node) {
  Symbol *var_sym = var_ref_node->var_ref.resolved_symbol;
  LLVMValueRef llvm_var;

  switch (var_sym->type) {
  case TYPE_INT: {
    llvm_var = LLVMBuildLoad2(components->builder,
                              LLVMInt32TypeInContext(components->ctx),
                              var_sym->llvm_val_ref, var_sym->name);
    break;
  }
  case TYPE_STRING: {
    LLVMTypeRef str_members[] = {
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        LLVMInt32TypeInContext(components->ctx)};
    LLVMTypeRef string_struct_type =
        LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    LLVMValueRef char_ptr_gep =
        LLVMBuildStructGEP2(components->builder, string_struct_type,
                            var_sym->llvm_val_ref, 0, "str_gep");
    llvm_var = LLVMBuildLoad2(
        components->builder,
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        char_ptr_gep, var_sym->name);
    break;
  }

  case TYPE_FLOAT: {
    llvm_var = LLVMBuildLoad2(components->builder,
                              LLVMFloatTypeInContext(components->ctx),
                              var_sym->llvm_val_ref, var_sym->name);
    break;
  }
  case TYPE_BOOL: {
    llvm_var = LLVMBuildLoad2(components->builder,
                              LLVMInt1TypeInContext(components->ctx),
                              var_sym->llvm_val_ref, var_sym->name);
    break;
  }
  default:
    break;
  }

  return llvm_var;
}

void slice_string(Token string, char *clean_str) {
  const char *sliced_start = string.start + 1;

  int sliced_length = string.length - 1;
  memcpy(clean_str, sliced_start, sliced_length);

  clean_str[sliced_length] = '\0';
}

float str_to_int_k(const char *s, int k) {
  int result = 0;
  float final;
  int dp = 1;
  bool is_decimal_place = false;
  bool negative = false;

  for (int i = 0; i < k; i++) {
    if (s[i] == '-') {
      negative = true;
      continue;
    }
    if (s[i] == '.') {
      is_decimal_place = true;
      continue;
    }
    result = result * 10 + (s[i] - '0');

    if (is_decimal_place)
      dp *= 10;
  }

  final = (float)result / dp;

  return (negative) ? final * -1 : final;
}

LLVMValueRef arihmetics(LLVMComponents *components, ASTNode *node,
                        char *result_name) {
  if (!node)
    return NULL;

  if (node->Type == AST_LITERAL_EXPR) {
    int val = (int)str_to_int_k(node->literal_expr.token.start,
                                node->literal_expr.token.length);
    return LLVMConstInt(LLVMInt32TypeInContext(components->ctx), val, 0);
  }

  if (node->Type == AST_VAR_REF) {
    return load_variable(components, node);
  }if(node->Type == AST_CALL_FXN){
    return _apl_eval_function_call(components, node);
  }

  if (node->Type == AST_BINARY_EXPR) {
    LLVMValueRef left =
        arihmetics(components, node->binary_expr.left, "left_tmp");
    LLVMValueRef right =
        arihmetics(components, node->binary_expr.right, "right_tmp");

    switch (node->binary_expr.operator_type) {
    case TOKEN_ADD:
      return LLVMBuildAdd(components->builder, left, right, result_name);
    case TOKEN_SUB:
      return LLVMBuildSub(components->builder, left, right, result_name);
    case TOKEN_DIV:
      return LLVMBuildSDiv(components->builder, left, right, result_name);
    case TOKEN_MUL:
      return LLVMBuildMul(components->builder, left, right, result_name);

    // Comparisons
    case TOKEN_GT:
      return LLVMBuildICmp(components->builder, LLVMIntSGT, left, right,
                           result_name);
    case TOKEN_ST:
      return LLVMBuildICmp(components->builder, LLVMIntSLT, left, right,
                           result_name);
    case TOKEN_GE:
      return LLVMBuildICmp(components->builder, LLVMIntSGE, left, right,
                           result_name);
    case TOKEN_SE:
      return LLVMBuildICmp(components->builder, LLVMIntSLE, left, right,
                           result_name);
    case TOKEN_EQT:
      return LLVMBuildICmp(components->builder, LLVMIntEQ, left, right,
                           result_name);
    case TOKEN_NEQ:
      return LLVMBuildICmp(components->builder, LLVMIntNE, left, right,
                           result_name);

    // Conditions
    case TOKEN_AND:
      return LLVMBuildAnd(components->builder, left, right, result_name);
    case TOKEN_OR:
      return LLVMBuildOr(components->builder, left, right, result_name);
    default:
      return NULL;
    }

    return NULL;
  }
}

LLVMTypeRef _enquire_fxn_return_type(LLVMComponents *components, Fxn *fxn) {
  switch (fxn->return_type) {
  case TYPE_INT:
    return LLVMInt32TypeInContext(components->ctx);
  case TYPE_FLOAT:
    return LLVMFloatTypeInContext(components->ctx);
  case TYPE_STRING:
    return LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
  case TYPE_BOOL:
    return LLVMInt1TypeInContext(components->ctx);
  default:
    return LLVMVoidTypeInContext(components->ctx);
  }
}


__attribute__((always_inline)) float return_eval_int(ASTNode* expr){
    char int_str[expr->literal_expr.token.length + 1];
    memcpy(int_str, expr->literal_expr.token.start,
           expr->literal_expr.token.length + 1);
    int_str[expr->literal_expr.token.length] = '\0';
   
    return str_to_int_k(int_str, expr->literal_expr.token.length);

}

LLVMValueRef _apl_eval_function_call(LLVMComponents *components, ASTNode *node){
      LLVMValueRef result;
      FxnCallMetaData meta_data = node->call_fxn.resolved_symbol->fxn_meta_data;

      result = LLVMBuildCall2(components->builder,meta_data.fxn_type, meta_data.the_fxn, NULL, 0, "");

      return result;
}
