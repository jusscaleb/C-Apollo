#include "../../headers/llvm_backend.h"
#include <stdint.h>




__attribute__((always_inline)) void slice_string(Token string, char *clean_str) {
  const char *sliced_start = string.start + 1;

  int sliced_length = string.length - 1;
  memcpy(clean_str, sliced_start, sliced_length);

  clean_str[sliced_length] = '\0';
}

__attribute__((always_inline)) float str_to_int_k(const char *s, int k) {
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
    return LLVMConstInt(I32(components->ctx), val, 0);
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

  return NULL;
}

__attribute__((always_inline)) float return_eval_int(ASTNode* expr){
    char int_str[expr->literal_expr.token.length + 1];
    memcpy(int_str, expr->literal_expr.token.start,
           expr->literal_expr.token.length + 1);
    int_str[expr->literal_expr.token.length] = '\0';
   
    return str_to_int_k(int_str, expr->literal_expr.token.length);

}

__attribute__((always_inline)) LLVMValueRef _apl_eval_function_call(LLVMComponents *components, ASTNode *node){
      LLVMValueRef result;
      FxnCallMetaData meta_data = node->call_fxn.resolved_symbol->fxn_meta_data;
      uint32_t a_numbers = (node->call_fxn.args) ? _apl_get_n_args(components, node->call_fxn.args): 0;
      bool is_nested = (node->call_fxn.fxn.parent_fxn != NULL || (node->call_fxn.resolved_symbol && node->call_fxn.resolved_symbol->fxn && node->call_fxn.resolved_symbol->fxn->parent_fxn != NULL));
      uint32_t total_args = is_nested ? a_numbers + 1 : a_numbers;
      LLVMValueRef args[total_args > 0 ? total_args : 1];

      Args *a = node->call_fxn.args;

      for(int i = 0; i < a_numbers; i++){
        switch(a->datatype){
          case TYPE_INT:{
            if(a->arg->Type == AST_LITERAL_EXPR){
               args[i] = LLVMConstInt(_apl_get_llvm_type(components, a->datatype), (int)return_eval_int(a->arg), 0);
                break;
            }

            if(a->arg->Type == AST_BINARY_EXPR){
               args[i] = arihmetics(components, a->arg, "");
               break;
            }

            args[i] = load_variable(components, a->arg);
            break;
          }
          default:
            args[i] = load_variable(components, a->arg);
            break;
        }
        a = a->next;
      }

      if (is_nested) {
        LLVMValueRef frame_ptr = NULL;
        Fxn *target_parent = (node->call_fxn.resolved_symbol && node->call_fxn.resolved_symbol->fxn) 
            ? node->call_fxn.resolved_symbol->fxn->parent_fxn 
            : node->call_fxn.fxn.parent_fxn;

        if (target_parent && components->current_fxn_ast && target_parent == components->current_fxn_ast) {
          frame_ptr = components->current_frame_alloc;
        } else {
          frame_ptr = components->current_parent_frame ? components->current_parent_frame : components->current_frame_alloc;
        }

        if (!frame_ptr) {
          frame_ptr = LLVMConstNull(LLVMPointerType(I8(components->ctx), 0));
        } else {
          frame_ptr = LLVMBuildBitCast(components->builder, frame_ptr, LLVMPointerType(I8(components->ctx), 0), "");
        }
        args[a_numbers] = frame_ptr;
      }

      result = LLVMBuildCall2(components->builder, meta_data.fxn_type, meta_data.the_fxn, args, total_args, "");
     
      return result;
}

uint32_t _apl_get_n_params(LLVMComponents *components, Params *p){
  uint32_t n = 0;
  while(p != NULL){
    n++;
    p = p->next;
  }
  return n;
}

uint32_t _apl_get_n_args(LLVMComponents *components, Args *a){
  uint32_t n = 0;
  while(a != NULL){
    n++;
    a = a->next;
  }
  return n;
}


__attribute__((always_inline)) LLVMTypeRef _apl_get_llvm_type(LLVMComponents*components, DataType dt){
  switch(dt){
    case TYPE_INT: return I32(components->ctx);
    case TYPE_FLOAT: return F32(components->ctx);
    case TYPE_BOOL: return I1(components->ctx);
    case TYPE_NULL: return VOID(components->ctx);
    default: return LLVMPointerType(I8(components->ctx), 0);
  }
}

void _apl_build_string_reassign(LLVMComponents *components, LLVMValueRef var_ptr, char* char_ptr, LLVMTypeRef struct_type){
  LLVMTypeRef param_types[] = {LLVMPointerType(struct_type, 0),
     LLVMPointerType(I8(components->ctx), 0)};
    LLVMTypeRef create_str_fxn_type = LLVMFunctionType(
        VOID(components->ctx), param_types, 2, false);
    LLVMValueRef create_str_fxn =
        LLVMGetNamedFunction(components->module, "__apl_create_str__");
    LLVMValueRef llvm_str_lit =
        LLVMBuildGlobalStringPtr(components->builder, char_ptr, "str_lit");
    LLVMValueRef args[] = {var_ptr, llvm_str_lit};

    LLVMBuildCall2(components->builder, create_str_fxn_type, create_str_fxn, args, 2, "");
}
