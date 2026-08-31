#include "../../headers/llvm_backend.h"
#include <stdio.h>
#include <stdlib.h>

static LLVMTypeRef get_llvm_elem_type(LLVMContextRef ctx, DataType dt) {
  switch (dt) {
    case TYPE_INT:
    case TYPE_INT_ARRAY:
      return I32(ctx);
    case TYPE_FLOAT:
    case TYPE_FLOAT_ARRAY:
      return F32(ctx);
    case TYPE_BOOL:
    case TYPE_BOOL_ARRAY:
      return I1(ctx);
    case TYPE_CHAR:
    case TYPE_CHAR_ARRAY:
      return I8(ctx);
    case TYPE_STRING:
    case TYPE_STR_ARRAY:
      return LLVMPointerType(I8(ctx), 0);
    default:
      return I32(ctx);
  }
}

LLVMValueRef _apl_gen_array_literal(LLVMComponents *components, ASTNode *node) {
  if (!node || node->Type != AST_ARRAY_LITERAL) return NULL;

  LLVMContextRef ctx = components->ctx;
  LLVMBuilderRef builder = components->builder;
  uint32_t count = node->array_literal.count;
  DataType elem_datatype = node->array_literal.element_type;
  LLVMTypeRef elem_type = get_llvm_elem_type(ctx, elem_datatype);
  bool is_dynamic = node->array_literal.is_dynamic;

  if (!is_dynamic) {
    LLVMValueRef arena_to_use = components->target_return_arena;
    if (arena_to_use) {
      LLVMValueRef buf_size_bytes = LLVMBuildMul(builder, LLVMSizeOf(elem_type), LLVMConstInt(LLVMInt64TypeInContext(ctx), count, false), "buf_bytes");
      LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_grow_and_alloc");
      if (!fn) {
        LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(ctx), 0), LLVMInt64TypeInContext(ctx) };
        LLVMTypeRef fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(ctx), 0), param_types, 2, false);
        fn = LLVMAddFunction(components->module, "apl_arena_grow_and_alloc", fn_type);
      }
      LLVMValueRef raw_buf = LLVMBuildCall2(builder, LLVMGlobalGetValueType(fn), fn, (LLVMValueRef[]){ arena_to_use, buf_size_bytes }, 2, "raw_buf");
      LLVMValueRef arena_ptr = LLVMBuildBitCast(builder, raw_buf, LLVMPointerType(elem_type, 0), "arena_arr_ptr");

      for (uint32_t i = 0; i < count; i++) {
        ASTNode *elem_ast = node->array_literal.elements[i];
        LLVMValueRef elem_val = elem_ast ? arihmetics(components, elem_ast, "") : NULL;
        if (!elem_val) elem_val = LLVMConstNull(elem_type);

        LLVMValueRef elem_idx[1] = { LLVMConstInt(I32(ctx), i, false) };
        LLVMValueRef elem_gep = LLVMBuildGEP2(builder, elem_type, arena_ptr, elem_idx, 1, "elem_gep");
        LLVMBuildStore(builder, elem_val, elem_gep);
      }
      return arena_ptr;
    }

    LLVMTypeRef array_type = LLVMArrayType(elem_type, count);

    Symbol b0_sym;
    memset(&b0_sym, 0, sizeof(Symbol));
    b0_sym.name = "static_array";
    b0_sym.bucket = BUCKET_PLUS_ONE;

    LLVMValueRef alloca_ptr = _apl_allocate_variable_by_bucket(components, &b0_sym, array_type);

    for (uint32_t i = 0; i < count; i++) {
      ASTNode *elem_ast = node->array_literal.elements[i];
      LLVMValueRef elem_val = elem_ast ? arihmetics(components, elem_ast, "") : NULL;
      if (!elem_val) elem_val = LLVMConstNull(elem_type);

      LLVMValueRef indices[2] = {
          LLVMConstInt(I32(ctx), 0, false),
          LLVMConstInt(I32(ctx), i, false)
      };

      LLVMValueRef elem_gep = LLVMBuildGEP2(builder, array_type, alloca_ptr, indices, 2, "elem_gep");
      LLVMBuildStore(builder, elem_val, elem_gep);
    }

    return alloca_ptr;
  } else {
    uint32_t capacity = node->array_literal.capacity > 0 ? node->array_literal.capacity : count;

    LLVMTypeRef struct_fields[3] = {
        LLVMPointerType(elem_type, 0),
        I32(ctx),
        I32(ctx)
    };
    LLVMTypeRef array_hdr_type = LLVMStructTypeInContext(ctx, struct_fields, 3, false);

    Symbol b2_sym;
    memset(&b2_sym, 0, sizeof(Symbol));
    b2_sym.name = "dyn_array_hdr";
    b2_sym.bucket = BUCKET_TWO;

    LLVMValueRef raw_hdr_ptr = _apl_allocate_variable_by_bucket(components, &b2_sym, array_hdr_type);
    LLVMValueRef hdr_ptr = LLVMBuildBitCast(builder, raw_hdr_ptr, LLVMPointerType(array_hdr_type, 0), "dyn_hdr_ptr");

    LLVMValueRef buf_size_bytes = LLVMBuildMul(builder, LLVMSizeOf(elem_type), LLVMConstInt(LLVMInt64TypeInContext(ctx), capacity, false), "buf_bytes");
    LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_grow_and_alloc");
    if (!fn) {
      LLVMTypeRef param_types[] = { LLVMPointerType(I8(ctx), 0), LLVMInt64TypeInContext(ctx) };
      LLVMTypeRef fn_type = LLVMFunctionType(LLVMPointerType(I8(ctx), 0), param_types, 2, false);
      fn = LLVMAddFunction(components->module, "apl_arena_grow_and_alloc", fn_type);
    }
    LLVMValueRef arena_to_use = (components->target_return_arena != NULL) ? components->target_return_arena : components->current_arena_ptr;
    LLVMValueRef raw_buf = LLVMBuildCall2(builder, LLVMGlobalGetValueType(fn), fn, (LLVMValueRef[]){ arena_to_use, buf_size_bytes }, 2, "raw_buf");
    LLVMValueRef elements_buf = LLVMBuildBitCast(builder, raw_buf, LLVMPointerType(elem_type, 0), "elements_buf");

    for (uint32_t i = 0; i < count; i++) {
      ASTNode *elem_ast = node->array_literal.elements[i];
      LLVMValueRef elem_val = elem_ast ? arihmetics(components, elem_ast, "") : NULL;
      if (!elem_val) elem_val = LLVMConstNull(elem_type);

      LLVMValueRef elem_idx[1] = { LLVMConstInt(I32(ctx), i, false) };
      LLVMValueRef elem_gep = LLVMBuildGEP2(builder, elem_type, elements_buf, elem_idx, 1, "buf_elem_gep");
      LLVMBuildStore(builder, elem_val, elem_gep);
    }

    LLVMValueRef ptr_gep = LLVMBuildStructGEP2(builder, array_hdr_type, hdr_ptr, 0, "hdr_ptr_gep");
    LLVMBuildStore(builder, elements_buf, ptr_gep);

    LLVMValueRef len_gep = LLVMBuildStructGEP2(builder, array_hdr_type, hdr_ptr, 1, "hdr_len_gep");
    LLVMBuildStore(builder, LLVMConstInt(I32(ctx), count, false), len_gep);

    LLVMValueRef cap_gep = LLVMBuildStructGEP2(builder, array_hdr_type, hdr_ptr, 2, "hdr_cap_gep");
    LLVMBuildStore(builder, LLVMConstInt(I32(ctx), capacity, false), cap_gep);

    return hdr_ptr;
  }
}

LLVMValueRef _apl_gen_array_index_expr(LLVMComponents *components, ASTNode *node) {
  if (!node || node->Type != AST_INDEX_EXPR) return NULL;

  LLVMContextRef ctx = components->ctx;
  LLVMBuilderRef builder = components->builder;

  LLVMValueRef target_array_ptr = NULL;
  int known_len = -1;
  bool is_dyn = false;

  if (node->index_expr.target->Type == AST_VAR_REF) {
    Symbol *sym = node->index_expr.target->var_ref.resolved_symbol;
    if (sym) {
      target_array_ptr = load_variable(components, node->index_expr.target);
      known_len = sym->array_count;
      if (sym->bucket == BUCKET_TWO || sym->array_count == 0) {
        is_dyn = true;
      }
    }
  }
  if (!target_array_ptr) {
    target_array_ptr = arihmetics(components, node->index_expr.target, "target_arr");
  }
  if (!target_array_ptr) return NULL;

  LLVMValueRef idx_val = arihmetics(components, node->index_expr.index, "idx_offset");
  if (!idx_val) return NULL;

  DataType elem_dt = node->index_expr.type;
  if (elem_dt == TYPE_NULL && node->index_expr.target->Type == AST_VAR_REF) {
    Symbol *sym = node->index_expr.target->var_ref.resolved_symbol;
    if (sym) {
      if (sym->type == TYPE_INT_ARRAY || sym->type == TYPE_INT) elem_dt = TYPE_INT;
      else if (sym->type == TYPE_FLOAT_ARRAY || sym->type == TYPE_FLOAT) elem_dt = TYPE_FLOAT;
      else if (sym->type == TYPE_CHAR_ARRAY || sym->type == TYPE_CHAR) elem_dt = TYPE_CHAR;
      else if (sym->type == TYPE_BOOL_ARRAY || sym->type == TYPE_BOOL) elem_dt = TYPE_BOOL;
      else if (sym->type == TYPE_STR_ARRAY || sym->type == TYPE_STRING) elem_dt = TYPE_STRING;
    }
  }
  if (elem_dt == TYPE_NULL) elem_dt = TYPE_INT;

  LLVMTypeRef elem_type = get_llvm_elem_type(ctx, elem_dt);

  if (is_dyn) {
    LLVMTypeRef struct_fields[3] = {
        LLVMPointerType(elem_type, 0),
        I32(ctx),
        I32(ctx)
    };
    LLVMTypeRef array_hdr_type = LLVMStructTypeInContext(ctx, struct_fields, 3, false);
    LLVMValueRef ptr_gep = LLVMBuildStructGEP2(builder, array_hdr_type, target_array_ptr, 0, "hdr_ptr_gep");
    LLVMValueRef elem_buf_ptr = LLVMBuildLoad2(builder, LLVMPointerType(elem_type, 0), ptr_gep, "elem_buf_ptr");
    LLVMValueRef len_gep = LLVMBuildStructGEP2(builder, array_hdr_type, target_array_ptr, 1, "hdr_len_gep");
    LLVMValueRef dyn_len = LLVMBuildLoad2(builder, I32(ctx), len_gep, "hdr_len");

    LLVMValueRef is_valid = LLVMBuildICmp(builder, LLVMIntULT, idx_val, dyn_len, "is_valid");

    LLVMValueRef panic_fn = LLVMGetNamedFunction(components->module, "_apl_panic_out_of_bounds");
    if (!panic_fn) {
      LLVMTypeRef param_types[] = { I32(ctx), I32(ctx) };
      LLVMTypeRef fn_type = LLVMFunctionType(VOID(ctx), param_types, 2, false);
      panic_fn = LLVMAddFunction(components->module, "_apl_panic_out_of_bounds", fn_type);
    }

    LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(builder);
    LLVMValueRef parent_fn = LLVMGetBasicBlockParent(cur_bb);

    LLVMBasicBlockRef ok_bb = LLVMAppendBasicBlockInContext(ctx, parent_fn, "dyn_idx_ok");
    LLVMBasicBlockRef panic_bb = LLVMAppendBasicBlockInContext(ctx, parent_fn, "dyn_idx_panic");

    LLVMBuildCondBr(builder, is_valid, ok_bb, panic_bb);

    // Panic Block
    LLVMPositionBuilderAtEnd(builder, panic_bb);
    LLVMBuildCall2(builder, LLVMGlobalGetValueType(panic_fn), panic_fn, (LLVMValueRef[]){ idx_val, dyn_len }, 2, "");
    LLVMBuildUnreachable(builder);

    // OK Block
    LLVMPositionBuilderAtEnd(builder, ok_bb);
    LLVMValueRef elem_gep = LLVMBuildInBoundsGEP2(builder, elem_type, elem_buf_ptr, &idx_val, 1, "arr_idx_gep");
    return LLVMBuildLoad2(builder, elem_type, elem_gep, "arr_elem_val");
  }

  if (known_len > 0) {
    LLVMValueRef length_val = LLVMConstInt(I32(ctx), known_len, false);
    LLVMValueRef is_valid = LLVMBuildICmp(builder, LLVMIntULT, idx_val, length_val, "is_valid");

    LLVMValueRef panic_fn = LLVMGetNamedFunction(components->module, "_apl_panic_out_of_bounds");
    if (!panic_fn) {
      LLVMTypeRef param_types[] = { I32(ctx), I32(ctx) };
      LLVMTypeRef fn_type = LLVMFunctionType(VOID(ctx), param_types, 2, false);
      panic_fn = LLVMAddFunction(components->module, "_apl_panic_out_of_bounds", fn_type);
    }

    LLVMBasicBlockRef cur_bb = LLVMGetInsertBlock(builder);
    LLVMValueRef parent_fn = LLVMGetBasicBlockParent(cur_bb);

    LLVMBasicBlockRef ok_bb = LLVMAppendBasicBlockInContext(ctx, parent_fn, "idx_ok");
    LLVMBasicBlockRef panic_bb = LLVMAppendBasicBlockInContext(ctx, parent_fn, "idx_panic");

    LLVMBuildCondBr(builder, is_valid, ok_bb, panic_bb);

    // Panic Block (Cold Path - Immediate Exit)
    LLVMPositionBuilderAtEnd(builder, panic_bb);
    LLVMBuildCall2(builder, LLVMGlobalGetValueType(panic_fn), panic_fn, (LLVMValueRef[]){ idx_val, length_val }, 2, "");
    LLVMBuildUnreachable(builder);

    // OK Block (Hot Path - Normal Load)
    LLVMPositionBuilderAtEnd(builder, ok_bb);
    LLVMValueRef typed_ptr = LLVMBuildBitCast(builder, target_array_ptr, LLVMPointerType(elem_type, 0), "arr_ptr_cast");
    LLVMValueRef elem_gep = LLVMBuildInBoundsGEP2(builder, elem_type, typed_ptr, &idx_val, 1, "arr_idx_gep");
    return LLVMBuildLoad2(builder, elem_type, elem_gep, "arr_elem_val");
  }

  LLVMValueRef typed_ptr = LLVMBuildBitCast(builder, target_array_ptr, LLVMPointerType(elem_type, 0), "arr_ptr_cast");
  LLVMValueRef elem_gep = LLVMBuildInBoundsGEP2(builder, elem_type, typed_ptr, &idx_val, 1, "arr_idx_gep");
  return LLVMBuildLoad2(builder, elem_type, elem_gep, "arr_elem_val");
}
