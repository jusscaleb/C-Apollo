#include "../../headers/llvm_backend.h"
#include "../../headers/semantic.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdint.h>




void slice_string(Token string, char *clean_str) {
  const char *src = string.start + 1;
  const char *end = string.start + string.length;
  char *dst = clean_str;

  while (src < end) {
    if (*src == '\\' && (src + 1) < end) {
      src++;
      switch (*src) {
        case 'n':  *dst++ = '\n'; break;
        case 't':  *dst++ = '\t'; break;
        case 'r':  *dst++ = '\r'; break;
        case '0':  *dst++ = '\0'; break;
        case '\\': *dst++ = '\\'; break;
        case '"':  *dst++ = '"';  break;
        case '\'': *dst++ = '\''; break;
        default:   *dst++ = *src;  break;
      }
    } else {
      *dst++ = *src;
    }
    src++;
  }
  *dst = '\0';
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

uint8_t parse_char_literal(Token token) {
  const char *s = token.start;
  if (s[1] == '\\') {
    switch (s[2]) {
      case 'n':  return '\n';
      case 't':  return '\t';
      case 'r':  return '\r';
      case '0':  return '\0';
      case '\\': return '\\';
      case '\'': return '\'';
      case '"':  return '"';
      default:   return (uint8_t)s[2];
    }
  }
  return (uint8_t)s[1];
}

LLVMValueRef arihmetics(LLVMComponents *components, ASTNode *node,
                        char *result_name) {
  if (!node)
    return NULL;

  if (node->Type == AST_LITERAL_EXPR) {
    if (node->literal_expr.token.type == TOKEN_CHAR) {
      uint8_t c = parse_char_literal(node->literal_expr.token);
      return LLVMConstInt(I8(components->ctx), c, 0);
    }

    int val = (int)str_to_int_k(node->literal_expr.token.start,
                                node->literal_expr.token.length);
    return LLVMConstInt(I32(components->ctx), val, 0);
  }

  
  if (node->Type == AST_VAR_REF) {
    return load_variable(components, node);
  }
  if (node->Type == AST_CALL_FXN) {
    return _apl_eval_function_call(components, node);
  }

  if (node->Type == AST_URINARY_EXPR) {
    switch (node->urinary_expr.operator_type) {
    case TOKEN_REF: {
      ASTNode *var_node = node->urinary_expr.value;
      if (var_node && var_node->Type == AST_VAR_REF && var_node->var_ref.resolved_symbol) {
        return var_node->var_ref.resolved_symbol->llvm_val_ref;
      }
      return NULL;
    }
    case TOKEN_MUL:
    case TOKEN_PTR: {
      LLVMValueRef ptr_val = arihmetics(components, node->urinary_expr.value, "ptr_tmp");
      if (!ptr_val)
        return NULL;

      ASTNode *inner_node = node->urinary_expr.value;
      int inner_ptr_level = 0;
      DataType base_dt = TYPE_INT;

      if (inner_node && inner_node->Type == AST_VAR_REF && inner_node->var_ref.resolved_symbol) {
        inner_ptr_level = inner_node->var_ref.resolved_symbol->pointer_level;
        base_dt = inner_node->var_ref.resolved_symbol->type;
      } else if (inner_node && inner_node->Type == AST_URINARY_EXPR) {
        ASTNode *curr = inner_node;
        int level_offset = 0;
        while (curr && curr->Type == AST_URINARY_EXPR) {
          if (curr->urinary_expr.operator_type == TOKEN_MUL || curr->urinary_expr.operator_type == TOKEN_PTR) {
            level_offset--;
          } else if (curr->urinary_expr.operator_type == TOKEN_REF) {
            level_offset++;
          }
          curr = curr->urinary_expr.value;
        }
        if (curr && curr->Type == AST_VAR_REF && curr->var_ref.resolved_symbol) {
          inner_ptr_level = curr->var_ref.resolved_symbol->pointer_level + level_offset;
          base_dt = curr->var_ref.resolved_symbol->type;
        }
      }

      LLVMTypeRef load_type;
      if (inner_ptr_level > 1) {
        load_type = LLVMPointerType(I8(components->ctx), 0);
      } else {
        load_type = _apl_get_llvm_type(components, base_dt);
      }
      return LLVMBuildLoad2(components->builder, load_type, ptr_val, result_name);
    }
    case TOKEN_SUB: {
      LLVMValueRef val = arihmetics(components, node->urinary_expr.value, "sub_tmp");
      return LLVMBuildNeg(components->builder, val, result_name);
    }
    default:
      return NULL;
    }
  }

  if (node->Type == AST_BINARY_EXPR) {
    LLVMValueRef left =
        arihmetics(components, node->binary_expr.left, "left_tmp");
    LLVMValueRef right =
        arihmetics(components, node->binary_expr.right, "right_tmp");

    LLVMTypeRef left_type = LLVMTypeOf(left);
    LLVMTypeRef right_type = LLVMTypeOf(right);

    if (left_type != right_type) {
      if (LLVMGetTypeKind(left_type) == LLVMIntegerTypeKind &&
          LLVMGetTypeKind(right_type) == LLVMIntegerTypeKind) {
        uint32_t left_bw = LLVMGetIntTypeWidth(left_type);
        uint32_t right_bw = LLVMGetIntTypeWidth(right_type);
        if (left_bw < right_bw) {
          left = LLVMBuildZExt(components->builder, left, right_type, "");
        } else if (right_bw < left_bw) {
          right = LLVMBuildZExt(components->builder, right, left_type, "");
        }
      }
    }
    switch (node->binary_expr.operator_type) {

    //Arithmetics
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
      if (!node || !node->call_fxn.resolved_symbol) return NULL;

      LLVMValueRef result;
      FxnCallMetaData meta_data = node->call_fxn.resolved_symbol->fxn_meta_data;
      if (!meta_data.the_fxn || !meta_data.fxn_type) return NULL;

      uint32_t a_numbers = (node->call_fxn.args) ? _apl_get_n_args(components, node->call_fxn.args): 0;
      bool is_nested = (node->call_fxn.fxn.parent_fxn != NULL || (node->call_fxn.resolved_symbol && node->call_fxn.resolved_symbol->fxn && node->call_fxn.resolved_symbol->fxn->parent_fxn != NULL));
      bool is_b2_ret = (node->call_fxn.resolved_symbol && node->call_fxn.resolved_symbol->bucket == BUCKET_TWO);
      uint32_t total_args = a_numbers + (is_nested ? 1 : 0) + (is_b2_ret ? 1 : 0);
      LLVMValueRef args[total_args > 0 ? total_args : 1];

      Args *a = node->call_fxn.args;

      for(int i = 0; i < a_numbers && a != NULL; i++){
        if (!a->arg) {
          args[i] = LLVMConstNull(_apl_get_llvm_type(components, a->datatype));
          a = a->next;
          continue;
        }

        switch(a->arg->Type) {
          case AST_LITERAL_EXPR: {
            switch(a->datatype) {
              case TYPE_INT:
                args[i] = LLVMConstInt(_apl_get_llvm_type(components, a->datatype), (int)return_eval_int(a->arg), 0);
                break;
              case TYPE_FLOAT:
                args[i] = LLVMConstReal(F32(components->ctx), return_eval_int(a->arg));
                break;
              case TYPE_BOOL: {
                uint8_t b = (a->arg->literal_expr.token.length >= 4) ? 1 : 0;
                args[i] = LLVMConstInt(I1(components->ctx), b, 0);
                break;
              }
              case TYPE_CHAR: {
                uint8_t c = parse_char_literal(a->arg->literal_expr.token);
                args[i] = LLVMConstInt(I8(components->ctx), c, 0);
                break;
              }
              case TYPE_STRING: {
                const uint32_t LENGTH = a->arg->literal_expr.token.length;
                char str[LENGTH + 1];
                slice_string(a->arg->literal_expr.token, str);
                args[i] = LLVMBuildGlobalStringPtr(components->builder, str, "");
                break;
              }
              default:
                args[i] = LLVMConstInt(_apl_get_llvm_type(components, a->datatype), (int)return_eval_int(a->arg), 0);
                break;
            }
            break;
          }
          case AST_VAR_REF:
            args[i] = load_variable(components, a->arg);
            break;
          case AST_BINARY_EXPR:
          case AST_URINARY_EXPR:
            args[i] = arihmetics(components, a->arg, "");
            break;
          case AST_CALL_FXN:
            args[i] = _apl_eval_function_call(components, a->arg);
            break;
          default:
            args[i] = load_variable(components, a->arg);
            break;
        }
        a = a->next;
      }

      uint32_t arg_idx = a_numbers;
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
        args[arg_idx++] = frame_ptr;
      }
      if (is_b2_ret) {
        LLVMValueRef active_arena = components->target_return_arena ? components->target_return_arena : components->current_arena_ptr;
        args[arg_idx++] = active_arena;
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
    case TYPE_CHAR: return I8(components->ctx);
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
