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

ALWAYS_INLINE float str_to_int_k(const char *s, int k) {
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

LLVMValueRef _apl_get_lvalue_address(LLVMComponents *components, ASTNode *node) {
  if (!node) return NULL;

  if (node->Type == AST_VAR_REF) {
    if (node->var_ref.resolved_symbol) {
      return node->var_ref.resolved_symbol->llvm_val_ref;
    }
    return NULL;
  }

  if (node->Type == AST_ACCESS) {
    return _apl_get_struct_access_ptr(components, node);
  }

  if (node->Type == AST_INDEX_EXPR) {
    LLVMValueRef target_array_ptr = NULL;
    DataType elem_dt = TYPE_INT;
    bool is_dyn = false;
    int known_len = -1;
    if (node->index_expr.target->Type == AST_VAR_REF) {
      Symbol *sym = node->index_expr.target->var_ref.resolved_symbol;
      if (sym) {
        target_array_ptr = load_variable(components, node->index_expr.target);
        known_len = sym->array_count;
        if (sym->type == TYPE_INT_ARRAY || sym->type == TYPE_INT) elem_dt = TYPE_INT;
        else if (sym->type == TYPE_FLOAT_ARRAY || sym->type == TYPE_FLOAT) elem_dt = TYPE_FLOAT;
        else if (sym->type == TYPE_CHAR_ARRAY || sym->type == TYPE_CHAR) elem_dt = TYPE_CHAR;
        else if (sym->type == TYPE_BOOL_ARRAY || sym->type == TYPE_BOOL) elem_dt = TYPE_BOOL;
        else if (sym->type == TYPE_STR_ARRAY || sym->type == TYPE_STRING) elem_dt = TYPE_STRING;
        else if (sym->type == TYPE_STRUCT) elem_dt = TYPE_STRUCT;

        if (sym->bucket == BUCKET_TWO || sym->array_count == 0) {
          is_dyn = true;
        }
      }
    }
    if (node->index_expr.type != TYPE_NULL) {
      elem_dt = node->index_expr.type;
    }
    if (!target_array_ptr) {
      target_array_ptr = arihmetics(components, node->index_expr.target, "target_arr");
    }
    if (!target_array_ptr) return NULL;

    LLVMValueRef idx_val = arihmetics(components, node->index_expr.index, "idx_offset");
    if (!idx_val) return NULL;

    Symbol *target_sym = (node->index_expr.target->Type == AST_VAR_REF) ? node->index_expr.target->var_ref.resolved_symbol : NULL;
    LLVMTypeRef elem_type = (elem_dt == TYPE_STRUCT && target_sym && target_sym->llvm_struct_type)
                                ? target_sym->llvm_struct_type
                                : _apl_get_llvm_type(components, elem_dt);
    if (!elem_type || LLVMGetTypeKind(elem_type) == LLVMVoidTypeKind) elem_type = I32(components->ctx);

    if (is_dyn) {
      LLVMTypeRef struct_fields[3] = {
          LLVMPointerType(elem_type, 0),
          I32(components->ctx),
          I32(components->ctx)
      };
      LLVMTypeRef array_hdr_type = LLVMStructTypeInContext(components->ctx, struct_fields, 3, false);
      LLVMValueRef ptr_gep = LLVMBuildStructGEP2(components->builder, array_hdr_type, target_array_ptr, 0, "hdr_ptr_gep");
      LLVMValueRef elem_buf_ptr = LLVMBuildLoad2(components->builder, LLVMPointerType(elem_type, 0), ptr_gep, "elem_buf_ptr");
      LLVMValueRef len_gep = LLVMBuildStructGEP2(components->builder, array_hdr_type, target_array_ptr, 1, "hdr_len_gep");
      LLVMValueRef dyn_len = LLVMBuildLoad2(components->builder, I32(components->ctx), len_gep, "hdr_len");

      LLVMValueRef is_valid = LLVMBuildICmp(components->builder, LLVMIntULT, idx_val, dyn_len, "is_valid");

      LLVMValueRef panic_fn = LLVMGetNamedFunction(components->module, "_apl_panic_out_of_bounds");
      if (!panic_fn) {
        LLVMTypeRef param_types[] = { I32(components->ctx), I32(components->ctx) };
        LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 2, false);
        panic_fn = LLVMAddFunction(components->module, "_apl_panic_out_of_bounds", fn_type);
      }

      LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(components->builder);
      LLVMValueRef parent_fn = LLVMGetBasicBlockParent(cur_bb);

      LLVMBasicBlockRef ok_bb = LLVMAppendBasicBlockInContext(components->ctx, parent_fn, "dyn_idx_ok");
      LLVMBasicBlockRef panic_bb = LLVMAppendBasicBlockInContext(components->ctx, parent_fn, "dyn_idx_panic");

      LLVMBuildCondBr(components->builder, is_valid, ok_bb, panic_bb);

      // Panic Block
      LLVMPositionBuilderAtEnd(components->builder, panic_bb);
      LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(panic_fn), panic_fn, (LLVMValueRef[]){ idx_val, dyn_len }, 2, "");
      LLVMBuildUnreachable(components->builder);

      // OK Block
      LLVMPositionBuilderAtEnd(components->builder, ok_bb);
      return LLVMBuildInBoundsGEP2(components->builder, elem_type, elem_buf_ptr, &idx_val, 1, "arr_elem_ptr");
    }

    if (known_len > 0) {
      LLVMValueRef length_val = LLVMConstInt(I32(components->ctx), known_len, false);
      LLVMValueRef is_valid = LLVMBuildICmp(components->builder, LLVMIntULT, idx_val, length_val, "is_valid");

      LLVMValueRef panic_fn = LLVMGetNamedFunction(components->module, "_apl_panic_out_of_bounds");
      if (!panic_fn) {
        LLVMTypeRef param_types[] = { I32(components->ctx), I32(components->ctx) };
        LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 2, false);
        panic_fn = LLVMAddFunction(components->module, "_apl_panic_out_of_bounds", fn_type);
      }

      LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(components->builder);
      LLVMValueRef parent_fn = LLVMGetBasicBlockParent(cur_bb);

      LLVMBasicBlockRef ok_bb = LLVMAppendBasicBlockInContext(components->ctx, parent_fn, "idx_ok");
      LLVMBasicBlockRef panic_bb = LLVMAppendBasicBlockInContext(components->ctx, parent_fn, "idx_panic");

      LLVMBuildCondBr(components->builder, is_valid, ok_bb, panic_bb);

      // Panic Block
      LLVMPositionBuilderAtEnd(components->builder, panic_bb);
      LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(panic_fn), panic_fn, (LLVMValueRef[]){ idx_val, length_val }, 2, "");
      LLVMBuildUnreachable(components->builder);

      // OK Block
      LLVMPositionBuilderAtEnd(components->builder, ok_bb);
      LLVMValueRef typed_ptr = LLVMBuildBitCast(components->builder, target_array_ptr, LLVMPointerType(elem_type, 0), "arr_ptr_cast");
      return LLVMBuildInBoundsGEP2(components->builder, elem_type, typed_ptr, &idx_val, 1, "arr_elem_ptr");
    }

    LLVMValueRef typed_ptr = LLVMBuildBitCast(components->builder, target_array_ptr, LLVMPointerType(elem_type, 0), "arr_ptr_cast");
    return LLVMBuildInBoundsGEP2(components->builder, elem_type, typed_ptr, &idx_val, 1, "arr_elem_ptr");
  }

  if (node->Type == AST_URINARY_EXPR) {
    if (node->urinary_expr.operator_type == TOKEN_MUL || node->urinary_expr.operator_type == TOKEN_PTR) {
      return arihmetics(components, node->urinary_expr.value, "ptr_val");
    }
    if (node->urinary_expr.operator_type == TOKEN_REF) {
      return _apl_get_lvalue_address(components, node->urinary_expr.value);
    }
  }

  return NULL;
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
    if (node->literal_expr.token.type == TOKEN_FLOAT) {
      float f_val = str_to_int_k(node->literal_expr.token.start,
                                 node->literal_expr.token.length);
      return LLVMConstReal(F32(components->ctx), f_val);
    }
    if (node->literal_expr.token.type == TOKEN_BOOL) {
      int b_val = (node->literal_expr.token.length == 5) ? 0 : 1;
      return LLVMConstInt(I1(components->ctx), b_val, 0);
    }

    int val = (int)str_to_int_k(node->literal_expr.token.start,
                                node->literal_expr.token.length);
    return LLVMConstInt(I32(components->ctx), val, 0);
  }

  
  if (node->Type == AST_ARRAY_LITERAL) {
    return _apl_gen_array_literal(components, node);
  }

  if (node->Type == AST_INDEX_EXPR) {
    return _apl_gen_array_index_expr(components, node);
  }

  if (node->Type == AST_ACCESS) {
    return _apl_gen_struct_access_load(components, node);
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
      return _apl_get_lvalue_address(components, node->urinary_expr.value);
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
      } else if (inner_node && inner_node->Type == AST_INDEX_EXPR) {
        if (inner_node->index_expr.target && inner_node->index_expr.target->Type == AST_VAR_REF &&
            inner_node->index_expr.target->var_ref.resolved_symbol) {
          inner_ptr_level = inner_node->index_expr.target->var_ref.resolved_symbol->pointer_level;
          base_dt = inner_node->index_expr.target->var_ref.resolved_symbol->type;
        }
        if (inner_node->index_expr.type != TYPE_NULL) {
          base_dt = inner_node->index_expr.type;
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
      if (!val) return NULL;
      if (LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMFloatTypeKind ||
          LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMDoubleTypeKind) {
        return LLVMBuildFNeg(components->builder, val, result_name);
      }
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

    if (!left || !right)
      return NULL;

    LLVMTypeRef left_type = LLVMTypeOf(left);
    LLVMTypeRef right_type = LLVMTypeOf(right);

    LLVMTypeKind left_kind = LLVMGetTypeKind(left_type);
    LLVMTypeKind right_kind = LLVMGetTypeKind(right_type);

    bool is_float = (left_kind == LLVMFloatTypeKind || left_kind == LLVMDoubleTypeKind ||
                     right_kind == LLVMFloatTypeKind || right_kind == LLVMDoubleTypeKind);

    if (is_float) {
      if (left_kind == LLVMIntegerTypeKind) {
        left = LLVMBuildSIToFP(components->builder, left, F32(components->ctx), "promoted_left");
      }
      if (right_kind == LLVMIntegerTypeKind) {
        right = LLVMBuildSIToFP(components->builder, right, F32(components->ctx), "promoted_right");
      }

      switch (node->binary_expr.operator_type) {
      // Float Arithmetic
      case TOKEN_ADD:
        return LLVMBuildFAdd(components->builder, left, right, result_name);
      case TOKEN_SUB:
        return LLVMBuildFSub(components->builder, left, right, result_name);
      case TOKEN_DIV:
        return LLVMBuildFDiv(components->builder, left, right, result_name);
      case TOKEN_MUL:
        return LLVMBuildFMul(components->builder, left, right, result_name);
      case TOKEN_MOD:
        return LLVMBuildFRem(components->builder, left, right, result_name);

      // Float Comparisons
      case TOKEN_GT:
        return LLVMBuildFCmp(components->builder, LLVMRealOGT, left, right, result_name);
      case TOKEN_ST:
        return LLVMBuildFCmp(components->builder, LLVMRealOLT, left, right, result_name);
      case TOKEN_GE:
        return LLVMBuildFCmp(components->builder, LLVMRealOGE, left, right, result_name);
      case TOKEN_SE:
        return LLVMBuildFCmp(components->builder, LLVMRealOLE, left, right, result_name);
      case TOKEN_EQT:
        return LLVMBuildFCmp(components->builder, LLVMRealOEQ, left, right, result_name);
      case TOKEN_NEQ:
        return LLVMBuildFCmp(components->builder, LLVMRealONE, left, right, result_name);
      default:
        return NULL;
      }
    }

    if (left_type != right_type) {
      if (left_kind == LLVMIntegerTypeKind && right_kind == LLVMIntegerTypeKind) {
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
    case TOKEN_MOD:
      return LLVMBuildSRem(components->builder, left, right, result_name);

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

ALWAYS_INLINE float return_eval_int(ASTNode* expr){
    char int_str[256];
    int len = expr->literal_expr.token.length;
    if (len >= 256) len = 255;
    memcpy(int_str, expr->literal_expr.token.start, len);
    int_str[len] = '\0';
   
    return str_to_int_k(int_str, expr->literal_expr.token.length);

}

LLVMValueRef _apl_eval_function_call(LLVMComponents *components, ASTNode *node){
      if (!node) return NULL;

      LLVMValueRef the_fxn = NULL;
      LLVMTypeRef fxn_type = NULL;
      if (node->call_fxn.resolved_symbol) {
        the_fxn = node->call_fxn.resolved_symbol->fxn_meta_data.the_fxn;
        fxn_type = node->call_fxn.resolved_symbol->fxn_meta_data.fxn_type;
      }
      if (!the_fxn && node->call_fxn.name) {
        the_fxn = LLVMGetNamedFunction(components->module, node->call_fxn.name);
        if (the_fxn) {
          fxn_type = LLVMGlobalGetValueType(the_fxn);
        }
      }
      if (!the_fxn || !fxn_type) return NULL;

      uint32_t a_numbers = (node->call_fxn.args) ? _apl_get_n_args(components, node->call_fxn.args): 0;
      Fxn *target_fxn = (node->call_fxn.resolved_symbol && node->call_fxn.resolved_symbol->fxn) ? node->call_fxn.resolved_symbol->fxn : &node->call_fxn.fxn;
      bool is_nested = (target_fxn && target_fxn->parent_fxn != NULL &&
                        target_fxn->parent_fxn->name != NULL &&
                        strcmp(target_fxn->parent_fxn->name, "global") != 0);
      bool is_b2_ret = (node->call_fxn.fxn.bucket == BUCKET_TWO) ||
                       (node->call_fxn.resolved_symbol && (node->call_fxn.resolved_symbol->bucket == BUCKET_TWO || (node->call_fxn.resolved_symbol->fxn && node->call_fxn.resolved_symbol->fxn->bucket == BUCKET_TWO)));
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
            if (a->arg->var_ref.resolved_symbol && (a->arg->var_ref.resolved_symbol->type == TYPE_STRUCT || a->arg->var_ref.resolved_symbol->struct_fields)) {
              if (a->arg->var_ref.resolved_symbol->pointer_level > 0) {
                args[i] = load_variable(components, a->arg);
              } else {
                args[i] = _apl_get_lvalue_address(components, a->arg);
              }
            } else {
              args[i] = load_variable(components, a->arg);
            }
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

      LLVMValueRef result = LLVMBuildCall2(components->builder, fxn_type, the_fxn, args, total_args, "");
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


ALWAYS_INLINE LLVMTypeRef _apl_get_llvm_type(LLVMComponents*components, DataType dt){
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
