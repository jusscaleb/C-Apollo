#include "../../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void _apl_create_local_variable(LLVMComponents *components, ASTNode *var_node) {

  char var_name[var_node->var_decl.name_length + 1];
  memcpy(var_name, var_node->var_decl.name, var_node->var_decl.name_length);
  var_name[var_node->var_decl.name_length] = '\0';

  ASTNode *expr = var_node->var_decl.value;
  const DataType VAR_TYPE = var_node->var_decl.value_type;

  LLVMValueRef var_ptr;
  LLVMValueRef assign_val;
  LLVMValueRef var_val;
  LLVMValueRef val;

  if (var_node->var_decl.pointer_level > 0) {
    if (expr->Type == AST_URINARY_EXPR &&
        expr->urinary_expr.operator_type == TOKEN_REF) {
      ASTNode *ref_var = expr->urinary_expr.value;
      if (ref_var && ref_var->Type == AST_VAR_REF &&
          ref_var->var_ref.resolved_symbol) {
        LLVMValueRef target_addr =
            ref_var->var_ref.resolved_symbol->llvm_val_ref;
        val = LLVMBuildBitCast(components->builder, target_addr,
                               LLVMPointerType(I8(components->ctx), 0), "");
      } else {
        val = arihmetics(components, expr, "");
      }
    } else if (expr->Type == AST_VAR_REF) {
      val = load_variable(components, expr);
    } else if (expr->Type == AST_CALL_FXN) {
      val = _apl_eval_function_call(components, expr);
    } else {
      val = arihmetics(components, expr, "");
    }

    var_ptr = LLVMBuildAlloca(
        components->builder, LLVMPointerType(I8(components->ctx), 0), var_name);
    if (val) {
      LLVMBuildStore(components->builder, val, var_ptr);
    }
  } else {
    switch (VAR_TYPE) {
    case TYPE_BOOL: {
      var_ptr =
          LLVMBuildAlloca(components->builder, I1(components->ctx), var_name);

      if (expr->Type == AST_LITERAL_EXPR) {
        const int LENGTH = expr->literal_expr.token.length;
        int b_val = (LENGTH == 5) ? 0 : 1;
        val = LLVMConstInt(I1(components->ctx), b_val, 0);
      } else {
        val = arihmetics(components, expr, "");
      }

      LLVMBuildStore(components->builder, val, var_ptr);

      break;
    }
    case TYPE_FLOAT: {
      if (expr->Type == AST_LITERAL_EXPR) {
        float f_val = return_eval_int(expr);
        val = LLVMConstReal(F32(components->ctx), f_val);
      } else {
        val = arihmetics(components, expr, "");
      }

      var_ptr =
          LLVMBuildAlloca(components->builder, F32(components->ctx), var_name);
      LLVMBuildStore(components->builder, val, var_ptr);

      break;
    }
    case TYPE_INT: {
      if (expr->Type == AST_LITERAL_EXPR) {
        int i_val = (int)return_eval_int(expr);
        val = LLVMConstInt(I32(components->ctx), i_val, 0);
      } else {
        val = arihmetics(components, expr, "");

        if (LLVMTypeOf(val) != I32(components->ctx)) {
          val = LLVMBuildTrunc(components->builder, val, I32(components->ctx),
                               "");
        }
      }

      var_ptr =
          LLVMBuildAlloca(components->builder, I32(components->ctx), var_name);
      LLVMBuildStore(components->builder, val, var_ptr);
      break;
    }

    case TYPE_STRING: {
      char str[expr->literal_expr.token.length + 1];

      slice_string(expr->literal_expr.token, str);
      LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                   I32(components->ctx)};
      LLVMTypeRef string_struct_type =
          LLVMStructTypeInContext(components->ctx, str_members, 2, false);
      var_ptr =
          LLVMBuildAlloca(components->builder, string_struct_type, var_name);
      _apl_build_string_reassign(components, var_ptr, str, string_struct_type);

      break;
    }
    case TYPE_CHAR: {
      if (expr->Type == AST_LITERAL_EXPR) {
        uint8_t c = parse_char_literal(expr->literal_expr.token);
        val = LLVMConstInt(I8(components->ctx), c, false);
      } else if (expr->Type == AST_CALL_FXN) {
        val = _apl_eval_function_call(components, expr);
      } else if (expr->Type == AST_VAR_REF) {
        val = load_variable(components, expr);
      } else {
        val = arihmetics(components, expr, "");
      }
      var_ptr =
          LLVMBuildAlloca(components->builder, I8(components->ctx), var_name);
      LLVMBuildStore(components->builder, val, var_ptr);
      break;
    }
    default:
      break;
    }
  }
  var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;

  if (components->current_frame_alloc != NULL &&
      var_node->var_decl.resolved_symbol) {
    int idx = var_node->var_decl.resolved_symbol->frame_index;
    LLVMValueRef indices[] = {LLVMConstInt(I32(components->ctx), 0, false),
                              LLVMConstInt(I32(components->ctx), idx, false)};
    LLVMTypeRef arr_type =
        LLVMArrayType(LLVMPointerType(I8(components->ctx), 0), 128);
    LLVMValueRef slot =
        LLVMBuildGEP2(components->builder, arr_type,
                      components->current_frame_alloc, indices, 2, "");
    LLVMValueRef bitcast_val =
        LLVMBuildBitCast(components->builder, var_ptr,
                         LLVMPointerType(I8(components->ctx), 0), "");
    LLVMBuildStore(components->builder, bitcast_val, slot);
  }
}

void _apl_reassign_variable(LLVMComponents *components, ASTNode *node) {

  Symbol *var_sym = node->var_assign.resolved_symbol;
  DataType var_Type = var_sym->type;
  LLVMValueRef new_val = NULL;
  ASTNode *value_node = node->var_assign.value;

  LLVMValueRef var_target_ptr = var_sym ? var_sym->llvm_val_ref : NULL;

  bool is_parent_env =
      (var_sym && var_sym->fxn && components->current_fxn_ast &&
       var_sym->fxn->name[0] != '\0' &&
       components->current_fxn_ast->name[0] != '\0' &&
       strcmp(var_sym->fxn->name, components->current_fxn_ast->name) != 0 &&
       components->current_parent_frame != NULL);
  if (is_parent_env) {
    int idx = var_sym->frame_index;
    LLVMValueRef indices[] = {LLVMConstInt(I32(components->ctx), 0, false),
                              LLVMConstInt(I32(components->ctx), idx, false)};
    LLVMTypeRef arr_type =
        LLVMArrayType(LLVMPointerType(I8(components->ctx), 0), 128);
    LLVMValueRef slot =
        LLVMBuildGEP2(components->builder, arr_type,
                      components->current_parent_frame, indices, 2, "");
    LLVMValueRef raw_ptr = LLVMBuildLoad2(
        components->builder, LLVMPointerType(I8(components->ctx), 0), slot, "");

    LLVMTypeRef actual_type;
    if (var_sym->type == TYPE_STRING) {
      LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                   I32(components->ctx)};
      actual_type =
          LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    } else {
      actual_type = _apl_get_llvm_type(components, var_sym->type);
    }
    var_target_ptr = LLVMBuildBitCast(components->builder, raw_ptr,
                                      LLVMPointerType(actual_type, 0), "");
  }

  if (node->var_assign.deref_level > 0 && var_target_ptr != NULL) {
    for (int i = 0; i < node->var_assign.deref_level; i++) {
      var_target_ptr = LLVMBuildLoad2(components->builder,
                                      LLVMPointerType(I8(components->ctx), 0),
                                      var_target_ptr, "");
    }
    LLVMTypeRef elem_type = _apl_get_llvm_type(components, var_Type);
    var_target_ptr = LLVMBuildBitCast(components->builder, var_target_ptr,
                                      LLVMPointerType(elem_type, 0), "");
  }

  switch (var_Type) {
  case TYPE_INT: {
    if (value_node->Type == AST_LITERAL_EXPR) {
      int i_val = (int)return_eval_int(value_node);
      new_val = LLVMConstInt(I32(components->ctx), i_val, 0);
    } else if (value_node->Type == AST_BINARY_EXPR) {
      new_val = arihmetics(components, value_node, "");

    } else if (value_node->Type == AST_CALL_FXN) {
      new_val = _apl_eval_function_call(components, value_node);
    }
    break;
  }
  case TYPE_BOOL: {
    int b;
    const int LENGTH = value_node->literal_expr.token.length;

    b = (LENGTH == 5) ? 0 : 1;
    new_val = LLVMConstInt(I1(components->ctx), b, 0);

    break;
  }
  case TYPE_FLOAT: {
    float f_val = return_eval_int(value_node);
    new_val = LLVMConstReal(F32(components->ctx), f_val);

    break;
  }

  case TYPE_STRING: {

    char str[value_node->literal_expr.token.length + 1];

    slice_string(value_node->literal_expr.token, str);
    LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                 I32(components->ctx)};
    LLVMTypeRef string_struct_type =
        LLVMStructTypeInContext(components->ctx, str_members, 2, false);

    _apl_build_string_reassign(components, var_target_ptr, str,
                               string_struct_type);

    return;
  }
  case TYPE_CHAR: {
    if (value_node->Type == AST_LITERAL_EXPR) {
      uint8_t c = parse_char_literal(value_node->literal_expr.token);
      new_val = LLVMConstInt(I8(components->ctx), c, false);
    } else if (value_node->Type == AST_VAR_REF) {
      new_val = load_variable(components, value_node);
    } else if (value_node->Type == AST_CALL_FXN) {
      new_val = _apl_eval_function_call(components, value_node);
    }
    break;
  }
  default:
    break;
  }

  if (var_target_ptr && new_val) {
    LLVMBuildStore(components->builder, new_val, var_target_ptr);
  }
}
LLVMValueRef load_variable(LLVMComponents *components, ASTNode *var_ref_node) {
  if (!var_ref_node)
    return NULL;
  if (var_ref_node->Type == AST_URINARY_EXPR) {
    return arihmetics(components, var_ref_node, "");
  }
  Symbol *var_sym = var_ref_node->var_ref.resolved_symbol;
  if (!var_sym)
    return NULL;
  LLVMValueRef target_ptr = var_sym ? var_sym->llvm_val_ref : NULL;

  bool is_parent_env =
      (var_sym && var_sym->fxn && components->current_fxn_ast &&
       var_sym->fxn->name[0] != '\0' &&
       components->current_fxn_ast->name[0] != '\0' &&
       strcmp(var_sym->fxn->name, components->current_fxn_ast->name) != 0 &&
       components->current_parent_frame != NULL);
  if (is_parent_env) {
    int idx = var_sym->frame_index;
    LLVMValueRef indices[] = {LLVMConstInt(I32(components->ctx), 0, false),
                              LLVMConstInt(I32(components->ctx), idx, false)};
    LLVMTypeRef arr_type =
        LLVMArrayType(LLVMPointerType(I8(components->ctx), 0), 128);
    LLVMValueRef slot =
        LLVMBuildGEP2(components->builder, arr_type,
                      components->current_parent_frame, indices, 2, "");
    LLVMValueRef raw_ptr = LLVMBuildLoad2(
        components->builder, LLVMPointerType(I8(components->ctx), 0), slot, "");

    LLVMTypeRef actual_type;
    if (var_sym->type == TYPE_STRING) {
      LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                   I32(components->ctx)};
      actual_type =
          LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    } else {
      actual_type = _apl_get_llvm_type(components, var_sym->type);
    }
    target_ptr = LLVMBuildBitCast(components->builder, raw_ptr,
                                  LLVMPointerType(actual_type, 0), "");
  }

  LLVMValueRef llvm_var = NULL;

  if (var_sym && var_sym->pointer_level > 0) {
    LLVMTypeRef ptr_type = LLVMPointerType(I8(components->ctx), 0);
    if (!target_ptr) {
      LLVMValueRef alloc = LLVMBuildAlloca(components->builder, ptr_type, "");
      LLVMValueRef p_val =
          LLVMGetParam(components->current_fxn, var_sym->param_idx);
      LLVMBuildStore(components->builder, p_val, alloc);
      target_ptr = alloc;
      var_sym->llvm_val_ref = alloc;
    }
    llvm_var = LLVMBuildLoad2(components->builder, ptr_type, target_ptr,
                              var_sym->name);
  } else {
    switch (var_sym->type) {
    case TYPE_INT: {
      if (!target_ptr) {
        LLVMValueRef alloc =
            LLVMBuildAlloca(components->builder, I32(components->ctx), "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      llvm_var = LLVMBuildLoad2(components->builder, I32(components->ctx),
                                target_ptr, var_sym->name);
      break;
    }
    case TYPE_STRING: {
      if (!target_ptr) {
        llvm_var = LLVMBuildGlobalStringPtr(
            components->builder, var_sym->name ? var_sym->name : "", "str_val");
        break;
      }
      LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                   I32(components->ctx)};
      LLVMTypeRef string_struct_type =
          LLVMStructTypeInContext(components->ctx, str_members, 2, false);
      LLVMValueRef char_ptr_gep = LLVMBuildStructGEP2(
          components->builder, string_struct_type, target_ptr, 0, "str_gep");
      llvm_var = LLVMBuildLoad2(components->builder,
                                LLVMPointerType(I8(components->ctx), 0),
                                char_ptr_gep, var_sym->name);
      break;
    }

    case TYPE_FLOAT: {
      if (!target_ptr) {
        LLVMValueRef alloc =
            LLVMBuildAlloca(components->builder, F32(components->ctx), "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      llvm_var = LLVMBuildLoad2(components->builder, F32(components->ctx),
                                target_ptr, var_sym->name);
      break;
    }
    case TYPE_BOOL: {
      if (!target_ptr) {
        LLVMValueRef alloc =
            LLVMBuildAlloca(components->builder, I1(components->ctx), "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      llvm_var = LLVMBuildLoad2(components->builder, I1(components->ctx),
                                target_ptr, var_sym->name);
      break;
    }
    case TYPE_CHAR: {
      if (!target_ptr) {
        LLVMValueRef alloc =
            LLVMBuildAlloca(components->builder, I8(components->ctx), "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      llvm_var = LLVMBuildLoad2(components->builder, I8(components->ctx),
                                target_ptr, var_sym->name);
      break;
    }

    default:
      break;
    }
  }

  return llvm_var;
}