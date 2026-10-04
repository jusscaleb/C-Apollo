#include "../../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void _apl_populate_struct_from_literal(LLVMComponents *components,
                                              LLVMValueRef struct_ptr,
                                              LLVMTypeRef struct_type,
                                              ASTNode *expr) {
  if (!expr || expr->Type != AST_ARRAY_LITERAL || !struct_ptr || !struct_type)
    return;

  uint32_t count = expr->array_literal.count;
  ASTNode **elements = expr->array_literal.elements;

  for (uint32_t i = 0; i < count; i++) {
    if (!elements[i])
      continue;

    if (elements[i]->Type == AST_ARRAY_LITERAL) {
      LLVMValueRef field_gep = LLVMBuildStructGEP2(
          components->builder, struct_type, struct_ptr, i, "nested_struct_gep");
      LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(struct_type, i);
      _apl_populate_struct_from_literal(components, field_gep, field_type, elements[i]);
      continue;
    }

    if (elements[i]->Type == AST_VAR_REF && elements[i]->var_ref.resolved_symbol && elements[i]->var_ref.resolved_symbol->type == TYPE_STRUCT) {
      LLVMValueRef src_ptr = load_variable(components, elements[i]);
      LLVMTypeRef field_type = LLVMStructGetTypeAtIndex(struct_type, i);
      if (src_ptr && field_type) {
        LLVMValueRef loaded_struct = LLVMBuildLoad2(components->builder, field_type, src_ptr, "nested_struct_copy");
        LLVMValueRef field_gep = LLVMBuildStructGEP2(components->builder, struct_type, struct_ptr, i, "field_gep");
        LLVMBuildStore(components->builder, loaded_struct, field_gep);
      }
      continue;
    }

    LLVMValueRef elem_val = NULL;
    if (elements[i]->Type == AST_LITERAL_EXPR) {
      TokenType tt = elements[i]->literal_expr.token.type;
      if (tt == TOKEN_INT) {
        elem_val = LLVMConstInt(I32(components->ctx),
                                (int)return_eval_int(elements[i]), 0);
      } else if (tt == TOKEN_FLOAT) {
        elem_val =
            LLVMConstReal(F32(components->ctx), return_eval_int(elements[i]));
      } else if (tt == TOKEN_CHAR) {
        uint8_t c = parse_char_literal(elements[i]->literal_expr.token);
        elem_val = LLVMConstInt(I8(components->ctx), c, false);
      } else if (tt == TOKEN_BOOL) {
        int b = (elements[i]->literal_expr.token.length == 5) ? 0 : 1;
        elem_val = LLVMConstInt(I1(components->ctx), b, 0);
      } else if (tt == TOKEN_STRING) {
        char str[elements[i]->literal_expr.token.length +1];
        slice_string(elements[i]->literal_expr.token, str);
        LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                     I32(components->ctx)};
        LLVMTypeRef string_struct_type =
            LLVMStructTypeInContext(components->ctx, str_members, 2, false);
        LLVMValueRef str_alloc =
            LLVMBuildAlloca(components->builder, string_struct_type, "str_alloc");
        _apl_build_string_reassign(components, str_alloc, str,
                                   string_struct_type);
        elem_val = LLVMBuildLoad2(components->builder, string_struct_type,
                                  str_alloc, "str_load");
      } else {
        elem_val = arihmetics(components, elements[i], "");
      }
    } else if (elements[i]->Type == AST_VAR_REF) {
      elem_val = load_variable(components, elements[i]);
    } else if (elements[i]->Type == AST_CALL_FXN) {
      elem_val = _apl_eval_function_call(components, elements[i]);
    } else {
      elem_val = arihmetics(components, elements[i], "");
    }

    if (elem_val) {
      LLVMValueRef field_gep = LLVMBuildStructGEP2(
          components->builder, struct_type, struct_ptr, i, "field_gep");
      LLVMBuildStore(components->builder, elem_val, field_gep);
    }
  }
}

void _apl_create_local_variable(LLVMComponents *components, ASTNode *var_node) {

  int len_vname = var_node->var_decl.name_length;

  char var_name[len_vname+1];
  memcpy(var_name, var_node->var_decl.name, len_vname);
  var_name[len_vname] = '\0';

  ASTNode *expr = var_node->var_decl.value;
  const DataType VAR_TYPE = var_node->var_decl.value_type;

  Symbol *sym = var_node->var_decl.resolved_symbol;

  LLVMValueRef var_ptr;
  LLVMValueRef assign_val;
  LLVMValueRef var_val;
  LLVMValueRef val;

  if (expr && expr->Type == AST_ARRAY_LITERAL && VAR_TYPE != TYPE_STRUCT) {
    val = _apl_gen_array_literal(components, expr);
    var_ptr = val;
    if (var_node->var_decl.resolved_symbol) {
      var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
      var_node->var_decl.resolved_symbol->array_count =
          expr->array_literal.is_dynamic ? 0 : expr->array_literal.count;
    }
    return;
  }

  if (var_node->var_decl.pointer_level > 0) {
    if (expr) {
      if (expr->Type == AST_URINARY_EXPR &&
          expr->urinary_expr.operator_type == TOKEN_REF) {
        LLVMValueRef target_addr =
            _apl_get_lvalue_address(components, expr->urinary_expr.value);
        if (target_addr) {
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
    } else {
      val = LLVMConstNull(LLVMPointerType(I8(components->ctx), 0));
    }

    var_ptr = _apl_allocate_variable_by_bucket(
        components, sym, LLVMPointerType(I8(components->ctx), 0));
    if (val) {
      if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
        LLVMSetInitializer(var_ptr, val);
      } else {
        LLVMBuildStore(components->builder, val, var_ptr);
      }
    }
  } else {
    switch (VAR_TYPE) {
    case TYPE_BOOL: {
      var_ptr = _apl_allocate_variable_by_bucket(components, sym,
                                                 I1(components->ctx));
      if (expr) {
        if (expr->Type == AST_LITERAL_EXPR) {
          const int LENGTH = expr->literal_expr.token.length;
          int b_val = (LENGTH == 5) ? 0 : 1;
          val = LLVMConstInt(I1(components->ctx), b_val, 0);
        } else {
          val = arihmetics(components, expr, "");
        }
      } else {
        val = LLVMConstInt(I1(components->ctx), 0, 0);
      }

      if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
        LLVMSetInitializer(var_ptr, val);
      } else {
        LLVMBuildStore(components->builder, val, var_ptr);
      }

      break;
    }
    case TYPE_FLOAT: {
      if (expr) {
        if (expr->Type == AST_LITERAL_EXPR) {
          float f_val = return_eval_int(expr);
          val = LLVMConstReal(F32(components->ctx), f_val);
        } else {
          val = arihmetics(components, expr, "");
        }
      } else {
        val = LLVMConstReal(F32(components->ctx), 0.0);
      }

      var_ptr = _apl_allocate_variable_by_bucket(components, sym,
                                                 F32(components->ctx));
      if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
        LLVMSetInitializer(var_ptr, val);
      } else {
        LLVMBuildStore(components->builder, val, var_ptr);
      }

      break;
    }
    case TYPE_INT: {
      if (expr) {
        if (expr->Type == AST_LITERAL_EXPR) {
          int i_val = (int)return_eval_int(expr);
          val = LLVMConstInt(I32(components->ctx), i_val, 0);
        } else {
          val = arihmetics(components, expr, "");

          if (LLVMGetTypeKind(LLVMTypeOf(val)) == LLVMIntegerTypeKind && LLVMTypeOf(val) != I32(components->ctx)) {
            val = LLVMBuildTrunc(components->builder, val, I32(components->ctx),
                                 "");
          }
        }
      } else {
        val = LLVMConstInt(I32(components->ctx), 0, 0);
      }

      var_ptr = _apl_allocate_variable_by_bucket(components, sym,
                                                 I32(components->ctx));
      if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
        LLVMSetInitializer(var_ptr, val);
      } else {
        LLVMBuildStore(components->builder, val, var_ptr);
      }
      break;
    }

    case TYPE_STRING: {
      LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                   I32(components->ctx)};
      LLVMTypeRef string_struct_type =
          LLVMStructTypeInContext(components->ctx, str_members, 2, false);
      var_ptr =
          _apl_allocate_variable_by_bucket(components, sym, string_struct_type);

      if (expr) {
        char str[expr->literal_expr.token.length + 1];
        slice_string(expr->literal_expr.token, str);
        if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
          size_t str_len = strlen(str);
          LLVMValueRef str_const = LLVMConstStringInContext(components->ctx, str, (unsigned)str_len, false);
          char g_name[64];
          snprintf(g_name, sizeof(g_name), ".gstr_%s", sym->name ? sym->name : "anon");
          LLVMValueRef g_lit = LLVMAddGlobal(components->module, LLVMTypeOf(str_const), g_name);
          LLVMSetInitializer(g_lit, str_const);
          LLVMSetGlobalConstant(g_lit, true);
          LLVMSetLinkage(g_lit, LLVMPrivateLinkage);

          LLVMValueRef zero = LLVMConstInt(I32(components->ctx), 0, false);
          LLVMValueRef idxs[] = { zero, zero };
          LLVMValueRef str_ptr = LLVMConstGEP2(LLVMTypeOf(str_const), g_lit, idxs, 2);
          LLVMValueRef len_val = LLVMConstInt(I32(components->ctx), (unsigned)str_len, false);
          LLVMValueRef struct_members[] = { str_ptr, len_val };
          LLVMValueRef struct_init = LLVMConstStructInContext(components->ctx, struct_members, 2, false);
          LLVMSetInitializer(var_ptr, struct_init);
        } else {
          _apl_build_string_reassign(components, var_ptr, str, string_struct_type);
        }
      } else {
        val = LLVMConstNull(string_struct_type);
        if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
          LLVMSetInitializer(var_ptr, val);
        } else {
          LLVMBuildStore(components->builder, val, var_ptr);
        }
      }

      break;
    }
    case TYPE_CHAR: {
      if (expr) {
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
      } else {
        val = LLVMConstInt(I8(components->ctx), 0, false);
      }

      var_ptr = _apl_allocate_variable_by_bucket(components, sym,
                                                 I8(components->ctx));

      if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
        LLVMSetInitializer(var_ptr, val);
      } else {
        LLVMBuildStore(components->builder, val, var_ptr);
      }
      break;
    }
    case TYPE_INT_ARRAY:
    case TYPE_FLOAT_ARRAY:
    case TYPE_BOOL_ARRAY:
    case TYPE_CHAR_ARRAY:
    case TYPE_STR_ARRAY: {
      if (expr) {
        if (expr->Type == AST_ARRAY_LITERAL) {
          val = _apl_gen_array_literal(components, expr);
        } else if (expr->Type == AST_CALL_FXN) {
          val = _apl_eval_function_call(components, expr);
        } else if (expr->Type == AST_VAR_REF) {
          val = load_variable(components, expr);
        } else {
          val = arihmetics(components, expr, "");
        }
        if (sym && sym->llvm_val_ref && sym->llvm_val_ref != val) {
          LLVMBuildStore(components->builder, val, sym->llvm_val_ref);
          var_ptr = sym->llvm_val_ref;
        } else {
          var_ptr = val;
          if (var_node->var_decl.resolved_symbol) {
            var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
          }
        }
      } else {
        if (var_node->var_decl.array_count > 0) {
          DataType elem_dt = TYPE_INT;
          if (VAR_TYPE == TYPE_FLOAT_ARRAY) elem_dt = TYPE_FLOAT;
          else if (VAR_TYPE == TYPE_CHAR_ARRAY) elem_dt = TYPE_CHAR;
          else if (VAR_TYPE == TYPE_BOOL_ARRAY) elem_dt = TYPE_BOOL;
          else if (VAR_TYPE == TYPE_STR_ARRAY) elem_dt = TYPE_STRING;

          LLVMTypeRef elem_type = _apl_get_llvm_type(components, elem_dt);
          LLVMTypeRef arr_type = LLVMArrayType(elem_type, var_node->var_decl.array_count);
          var_ptr = _apl_allocate_variable_by_bucket(components, sym, arr_type);
        } else {
          var_ptr = NULL;
        }
        if (var_node->var_decl.resolved_symbol) {
          var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
        }
      }
      break;
    }
    case TYPE_STRUCT: {
      LLVMTypeRef struct_type = NULL;
      if (var_node->var_decl.struct_type_name) {
        struct_type = LLVMGetTypeByName(components->module, var_node->var_decl.struct_type_name);
      }
      if (!struct_type && sym && sym->llvm_struct_type) {
        struct_type = sym->llvm_struct_type;
      }
      if (!struct_type && sym && sym->name) {
        struct_type = LLVMGetTypeByName(components->module, sym->name);
      }
      if (!struct_type) {
        struct_type = LLVMPointerType(I8(components->ctx), 0);
      }
      if (sym) {
        sym->llvm_struct_type = struct_type;
      }

      var_ptr = _apl_allocate_variable_by_bucket(components, sym, struct_type);

      if (expr == NULL) {
        LLVMValueRef zero_init = LLVMConstNull(struct_type);
        if (sym && (sym->scope_level == 0 || sym->bucket == BUCKET_ONE)) {
          LLVMSetInitializer(var_ptr, zero_init);
        } else {
          LLVMBuildStore(components->builder, zero_init, var_ptr);
        }
      } else if (expr->Type == AST_ARRAY_LITERAL) {
        _apl_populate_struct_from_literal(components, var_ptr, struct_type, expr);
      } else if (expr->Type == AST_VAR_REF) {
        LLVMValueRef src_ptr = load_variable(components, expr);
        if (src_ptr && var_ptr) {
          LLVMValueRef loaded_struct = LLVMBuildLoad2(components->builder, struct_type, src_ptr, "struct_copy");
          LLVMBuildStore(components->builder, loaded_struct, var_ptr);
        }
      }
      break;
    }
    default:
      break;
    }
  }

  if (var_node->var_decl.resolved_symbol && var_ptr != NULL) {
    var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
  }

  if (components->current_frame_alloc != NULL && components->builder &&
      LLVMGetInsertBlock(components->builder) != NULL &&
      var_node->var_decl.resolved_symbol && var_ptr != NULL &&
      var_node->var_decl.resolved_symbol->scope_level > 0) {
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
  if (!node) return;

  if (node->var_assign.target_node != NULL) {
    LLVMValueRef target_addr = _apl_get_lvalue_address(components, node->var_assign.target_node);
    if (!target_addr) return;
    LLVMValueRef val = arihmetics(components, node->var_assign.value, "");
    if (val) {
      LLVMBuildStore(components->builder, val, target_addr);
    }
    return;
  }

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
  case TYPE_STRUCT: {
    LLVMTypeRef struct_type = (var_sym) ? var_sym->llvm_struct_type : NULL;
    if (!struct_type && var_sym) {
      struct_type = LLVMGetTypeByName(components->module, var_sym->name);
    }
    if (!struct_type) {
      struct_type = LLVMPointerType(I8(components->ctx), 0);
    }

    if (value_node->Type == AST_ARRAY_LITERAL) {
      _apl_populate_struct_from_literal(components, var_target_ptr, struct_type, value_node);
      return;
    } else if (value_node->Type == AST_VAR_REF) {
      LLVMValueRef src_ptr = load_variable(components, value_node);
      if (src_ptr && var_target_ptr) {
        LLVMValueRef loaded_struct = LLVMBuildLoad2(components->builder, struct_type, src_ptr, "struct_copy");
        LLVMBuildStore(components->builder, loaded_struct, var_target_ptr);
      }
      return;
    } else if (value_node->Type == AST_CALL_FXN) {
      new_val = _apl_eval_function_call(components, value_node);
    }
    break;
  }
  default:
    break;
  }

  if (var_target_ptr && new_val) {
    if (var_sym && var_sym->bucket == BUCKET_THREE) {
      // For ARC variables, we need to release the old pointer before
      // overwriting it. But only if the variable actually stores a pointer, or
      // if the user's memory model treats the heap block's contents as the ARC
      // target. Assuming BUCKET_THREE variables are heap pointers:
      if (var_sym->pointer_level > 0) {
        LLVMValueRef old_ptr = LLVMBuildLoad2(
            components->builder, LLVMPointerType(I8(components->ctx), 0),
            var_target_ptr, "old_ptr_val");
        _apl_emit_arc_release(components, old_ptr);

        if (value_node->Type == AST_VAR_REF) {
          LLVMValueRef new_ptr = LLVMBuildBitCast(
              components->builder, new_val,
              LLVMPointerType(I8(components->ctx), 0), "new_ptr_val");
          _apl_emit_arc_retain(components, new_ptr);
        }
      }
    }
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
      if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
        return NULL;
      }
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
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
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
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
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
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
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
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
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
    case TYPE_INT_ARRAY:
    case TYPE_FLOAT_ARRAY:
    case TYPE_BOOL_ARRAY:
    case TYPE_CHAR_ARRAY:
    case TYPE_STR_ARRAY: {
      if (!target_ptr) {
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
        LLVMTypeRef ptr_type = LLVMPointerType(I8(components->ctx), 0);
        LLVMValueRef alloc = LLVMBuildAlloca(components->builder, ptr_type, "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      if (target_ptr) {
        if (LLVMIsAAllocaInst(target_ptr) &&
            LLVMGetTypeKind(LLVMGetAllocatedType(target_ptr)) == LLVMPointerTypeKind) {
          llvm_var = LLVMBuildLoad2(components->builder,
                                    LLVMPointerType(I8(components->ctx), 0),
                                    target_ptr,
                                    var_sym->name ? var_sym->name : "");
        } else {
          llvm_var = LLVMBuildBitCast(components->builder, target_ptr,
                                      LLVMPointerType(I8(components->ctx), 0),
                                      var_sym->name ? var_sym->name : "");
        }
      }
      break;
    }
    case TYPE_STRUCT: {
      if (!target_ptr) {
        if (!components->current_fxn || var_sym->param_idx >= LLVMCountParams(components->current_fxn)) {
          return NULL;
        }
        LLVMTypeRef struct_type = var_sym->llvm_struct_type;
        if (!struct_type && var_sym->name) {
          struct_type = LLVMGetTypeByName(components->module, var_sym->name);
        }
        if (!struct_type) {
          struct_type = LLVMPointerType(I8(components->ctx), 0);
        }
        LLVMValueRef alloc = LLVMBuildAlloca(components->builder, struct_type, "");
        LLVMValueRef p_val =
            LLVMGetParam(components->current_fxn, var_sym->param_idx);
        LLVMBuildStore(components->builder, p_val, alloc);
        target_ptr = alloc;
        var_sym->llvm_val_ref = alloc;
      }
      if (target_ptr) {
        llvm_var = target_ptr;
      }
      break;
    }
    default:
      break;
    }
    }

    return llvm_var;
  }
