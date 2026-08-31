#include "../../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include <stdint.h>
#include <string.h>

/*================================================== INBUILT FUNCTIONS
 * =====================================================*/
void _apl_gen_println_ir(LLVMComponents *components, 
                         Args *args) {

  while (args != NULL) {
    DataType arg_type = args->datatype;
    ASTNode *expr = args->arg;
    LLVMTypeRef param_types[1];
    LLVMValueRef println_fxn = NULL, println_args = NULL;
    LLVMTypeRef func_type;

    //printf("POINTER LEVEL LLVM: %d\n", expr->call_fxn.resolved_symbol->pointer_level);
    bool is_ptr = (expr && expr->Type == AST_URINARY_EXPR && expr->urinary_expr.operator_type == TOKEN_REF) ||
                  (expr && expr->Type == AST_VAR_REF && expr->var_ref.resolved_symbol && expr->var_ref.resolved_symbol->pointer_level > 0) ||
                  (expr && expr->Type == AST_CALL_FXN && expr->call_fxn.resolved_symbol && expr->call_fxn.resolved_symbol->pointer_level > 0);


    if (is_ptr) {
      param_types[0] = LLVMPointerType(I8(components->ctx), 0);
      println_fxn = LLVMGetNamedFunction(components->module, "__apl_print_ptr");

      if (expr->Type == AST_URINARY_EXPR && expr->urinary_expr.operator_type == TOKEN_REF) {
        LLVMValueRef target_addr = _apl_get_lvalue_address(components, expr->urinary_expr.value);
        if (target_addr) {
          println_args = LLVMBuildBitCast(
              components->builder,
              target_addr,
              param_types[0],
              ""
          );
        }
      } else if (expr->Type == AST_VAR_REF) {
        println_args = load_variable(components, expr);
      } else if (expr->Type == AST_CALL_FXN) {
        println_args = _apl_eval_function_call(components, expr);
      }
    } else {
      switch (arg_type) {
      case TYPE_INT:
      case TYPE_FLOAT: {
        param_types[0] = _apl_get_llvm_type(components, arg_type);
        println_fxn = LLVMGetNamedFunction(
            components->module,
            (arg_type == TYPE_INT) ? "_apl_print_int" : "_apl_print_float");

        if (expr->Type == AST_VAR_REF) {
          println_args = load_variable(components, expr);
          break;
        }
        if (expr->Type == AST_BINARY_EXPR || expr->Type == AST_URINARY_EXPR || expr->Type == AST_INDEX_EXPR || expr->Type == AST_ACCESS) {
          println_args = arihmetics(components, expr, "");
          break;
        }
        if (expr->Type == AST_CALL_FXN) {
          println_args = _apl_eval_function_call(components, expr);
          break;
        }
        println_args =
            (arg_type == TYPE_INT)
                ? LLVMConstInt(param_types[0], (int)return_eval_int(expr), 0)
                : LLVMConstReal(param_types[0], return_eval_int(expr));
        break;
      }

      case TYPE_BOOL:
        param_types[0] = I1(components->ctx);
        println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_bool");

        if (expr->Type == AST_VAR_REF) {
          println_args = load_variable(components, expr);
          break;
        }
        if (expr->Type == AST_CALL_FXN) {
          println_args = _apl_eval_function_call(components, expr);
          break;
        }
        if (expr->Type == AST_ACCESS || expr->Type == AST_BINARY_EXPR || expr->Type == AST_URINARY_EXPR) {
          println_args = arihmetics(components, expr, "");
          break;
        }

        int b;
        const int LENGTH = expr->literal_expr.token.length;

        b = (LENGTH == 5) ? 0 : 1;
        println_args = LLVMConstInt(param_types[0], b, 0);
        break;

      case TYPE_CHAR: {
        param_types[0] = I8(components->ctx);

        println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_char");

        if (expr->Type == AST_VAR_REF) {
          println_args = load_variable(components, expr);
          break;
        }
        
        if (expr->Type == AST_BINARY_EXPR || expr->Type == AST_URINARY_EXPR || expr->Type == AST_INDEX_EXPR || expr->Type == AST_ACCESS) {
          println_args = arihmetics(components, expr, "");
          break;
        }

        if (expr->Type == AST_CALL_FXN) {
          println_args = _apl_eval_function_call(components, expr);
          break;
        }

        uint8_t c = parse_char_literal(expr->literal_expr.token);
        println_args = LLVMConstInt(param_types[0], c, 0);
        break;
      }

      case TYPE_STRING: {
        param_types[0] = LLVMPointerType(I8(components->ctx), 0);
        println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_string");

        if (expr->Type == AST_VAR_REF) {
          println_args = load_variable(components, expr);
          break;
        }
        if (expr->Type == AST_ACCESS || expr->Type == AST_BINARY_EXPR || expr->Type == AST_URINARY_EXPR) {
          println_args = arihmetics(components, expr, "");
          break;
        }
        if (expr->Type == AST_CALL_FXN) {
          println_args = _apl_eval_function_call(components, expr);
          break;
        }

        char str[expr->literal_expr.token.length + 1];

        slice_string(expr->literal_expr.token, str);
        println_args =
            LLVMBuildGlobalStringPtr(components->builder, str, "println_str");

        break;
      }

      case TYPE_NULL:
        break;

      default:
        break;
      }
    }

    if (println_fxn && println_args) {
      func_type = LLVMFunctionType(VOID(components->ctx), param_types, 1, 0);
      LLVMBuildCall2(components->builder, func_type, println_fxn, &println_args, 1, "");
    }

    args = args->next;
  }

  LLVMValueRef newline_fxn =
      LLVMGetNamedFunction(components->module, "_apl_print_newline");
  LLVMTypeRef newline_type =
      LLVMFunctionType(VOID(components->ctx), NULL, 0, false);
  LLVMBuildCall2(components->builder, newline_type, newline_fxn, NULL, 0, "");

  return;
}

void _apl_gen_function_start(LLVMComponents *components,
                              ASTNode *block_node) {
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMTypeRef main_fxn_type = LLVMFunctionType(
        I32(components->ctx), NULL, 0, false);
    LLVMValueRef main_fxn =
        LLVMAddFunction(components->module, "main", main_fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, main_fxn, "");
    LLVMPositionBuilderAtEnd(components->builder, entry);

    components->current_fxn = main_fxn;
    components->current_fxn_ast = &block_node->function.fxn;
    components->current_frame_alloc = LLVMBuildAlloca(
        components->builder,
        LLVMArrayType(LLVMPointerType(I8(components->ctx), 0), 128), "frame_env");
    components->current_arena_ptr = _apl_emit_arena_create(components, 0);
    components->target_return_arena = NULL;
    return;
  }

  else {
    components->current_fxn_ast = &block_node->function.fxn;
    bool is_nested = (block_node->function.fxn.parent_fxn != NULL &&
                      block_node->function.fxn.parent_fxn->name != NULL &&
                      strcmp(block_node->function.fxn.parent_fxn->name, "global") != 0);
    bool is_b2_ret = (block_node->function.fxn.bucket == BUCKET_TWO) || 
                     (block_node->function.resolved_symbol && block_node->function.resolved_symbol->bucket == BUCKET_TWO);

    uint32_t user_params =
        (block_node->function.fxn.params)
            ? _apl_get_n_params(components, block_node->function.fxn.params)
            : 0;
    uint32_t total_params = user_params + (is_nested ? 1 : 0) + (is_b2_ret ? 1 : 0);
    LLVMTypeRef param_types[total_params > 0 ? total_params : 1];

    Params *p = block_node->function.fxn.params;

    for (uint32_t i = 0; i < user_params; i++) {
      if (p && p->param && p->param->var_decl.pointer_level > 0) {
        param_types[i] = LLVMPointerType(I8(components->ctx), 0);
      } else {
        param_types[i] = _apl_get_llvm_type(components, p->param->var_decl.value_type);
      }
      p = p->next;
    }

    uint32_t curr_idx = user_params;
    if (is_nested) {
      param_types[curr_idx++] = LLVMPointerType(I8(components->ctx), 0);
    }
    if (is_b2_ret) {
      param_types[curr_idx++] = LLVMPointerType(I8(components->ctx), 0);
    }

    LLVMTypeRef ret_type;
    if (block_node->function.pointer_level > 0) {
      ret_type = LLVMPointerType(I8(components->ctx), 0);
    } else {
      ret_type = _apl_get_llvm_type(components, block_node->function.fxn.return_type);
    }

    LLVMTypeRef fxn_type = LLVMFunctionType(
        ret_type,
        (total_params == 0) ? NULL : param_types, total_params, false);
    LLVMValueRef fxn = LLVMAddFunction(components->module,
                                       block_node->function.name, fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, fxn, "");
    LLVMPositionBuilderAtEnd(components->builder, entry);

    if (block_node->function.resolved_symbol) {
      block_node->function.resolved_symbol->fxn_meta_data.fxn_type = fxn_type;
      block_node->function.resolved_symbol->fxn_meta_data.the_fxn = fxn;
    }
    components->current_fxn = fxn;
    components->current_frame_alloc = LLVMBuildAlloca(
        components->builder,
        LLVMArrayType(LLVMPointerType(I8(components->ctx), 0), 128), "frame_env");
    components->current_arena_ptr = _apl_emit_arena_create(components, 0);

    curr_idx = user_params;
    if (is_nested) {
      components->current_parent_frame = LLVMGetParam(fxn, curr_idx++);
    }
    if (is_b2_ret) {
      components->target_return_arena = LLVMGetParam(fxn, curr_idx++);
    } else {
      components->target_return_arena = NULL;
    }

    p = block_node->function.fxn.params;
    for (uint32_t i = 0; i < user_params; i++) {
      if (p && p->param && p->param->var_decl.resolved_symbol) {
        Symbol *sym = p->param->var_decl.resolved_symbol;
        char p_name[p->param->var_decl.name_length + 1];
        memcpy(p_name, p->param->var_decl.name, p->param->var_decl.name_length);
        p_name[p->param->var_decl.name_length] = '\0';

        LLVMValueRef param_val = LLVMGetParam(fxn, i);
        LLVMValueRef alloc = LLVMBuildAlloca(components->builder, param_types[i], p_name);
        LLVMBuildStore(components->builder, param_val, alloc);
        sym->llvm_val_ref = alloc;
      }
      p = p->next;
    }
  }
}

void _apl_gen_function_end(LLVMComponents *components, 
                           ASTNode *block_node) {

  LLVMBasicBlockRef current_block = (components->builder) ? LLVMGetInsertBlock(components->builder) : NULL;
  if (!current_block) return;

  if (LLVMGetBasicBlockTerminator(current_block)) return;

  if (components->current_arena_ptr) {
    _apl_emit_arena_destroy(components, components->current_arena_ptr);
  }

  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMBuildRet(
        components->builder,
        LLVMConstInt(I32(components->ctx), 0, false));
  } else if (block_node->function.fxn.return_type == TYPE_NULL) {
    LLVMBuildRetVoid(components->builder);
  } else {
    LLVMBuildUnreachable(components->builder);
  }
} 

void _apl_gen_return(LLVMComponents *components, 
                     ASTNode *node) {
  LLVMValueRef ret_val = NULL;
  ASTNode *val_node = node->ret_node.value;

  if (val_node) {
    if (val_node->Type == AST_VAR_REF) {
      ret_val = load_variable(components, val_node);
    } else if (val_node->Type == AST_ACCESS) {
      ret_val = _apl_gen_struct_access_load(components, val_node);
    } else if (val_node->Type == AST_INDEX_EXPR) {
      ret_val = _apl_gen_array_index_expr(components, val_node);
    } else if (val_node->Type == AST_CALL_FXN) {
      ret_val = _apl_eval_function_call(components, val_node);
    } else if (val_node->Type == AST_BINARY_EXPR || val_node->Type == AST_URINARY_EXPR) {
      ret_val = arihmetics(components, val_node, "");
    } else if (val_node->Type == AST_ARRAY_LITERAL) {
      ret_val = _apl_gen_array_literal(components, val_node);
      if (ret_val) {
        ret_val = LLVMBuildBitCast(components->builder, ret_val, LLVMPointerType(I8(components->ctx), 0), "");
      }
    } else {
      switch (node->ret_node.fxn.return_type) {
      case TYPE_INT_ARRAY:
      case TYPE_FLOAT_ARRAY:
      case TYPE_BOOL_ARRAY:
      case TYPE_CHAR_ARRAY:
      case TYPE_STR_ARRAY: {
        if (val_node->Type == AST_ARRAY_LITERAL) {
          ret_val = _apl_gen_array_literal(components, val_node);
        } else {
          ret_val = arihmetics(components, val_node, "");
        }
        if (ret_val) {
          ret_val = LLVMBuildBitCast(components->builder, ret_val, LLVMPointerType(I8(components->ctx), 0), "");
        }
        break;
      }
      case TYPE_INT: {
        uint32_t number = (int)return_eval_int(val_node);
        ret_val = LLVMConstInt(I32(components->ctx), number, false);
        break;
      }
      case TYPE_BOOL: {
        uint8_t b = (val_node->literal_expr.token.length >= 4) ? 1 : 0;
        ret_val = LLVMConstInt(I1(components->ctx), b, false);
        break;
      }
      case TYPE_STRING: {
        const uint32_t LENGTH = val_node->literal_expr.token.length;
        char str[LENGTH + 1];
        slice_string(val_node->literal_expr.token, str);
        ret_val = LLVMBuildGlobalStringPtr(components->builder, str, "");
        break;
      }
      case TYPE_FLOAT: {
        float number = return_eval_int(val_node);
        ret_val = LLVMConstReal(F32(components->ctx), number);
        break;
      }
      case TYPE_CHAR: {
        uint8_t c = parse_char_literal(val_node->literal_expr.token);
        ret_val = LLVMConstInt(I8(components->ctx), c, false);
        break;
      }
      default:
        break;
      }
    }
  }

  if (components->current_arena_ptr) {
    _apl_emit_arena_destroy(components, components->current_arena_ptr);
  }

  if (ret_val) {
    if (val_node && val_node->Type == AST_VAR_REF && 
        val_node->var_ref.resolved_symbol && 
        val_node->var_ref.resolved_symbol->bucket == BUCKET_THREE &&
        val_node->var_ref.resolved_symbol->pointer_level > 0) {
        
        LLVMValueRef ptr_val = LLVMBuildBitCast(components->builder, ret_val, LLVMPointerType(I8(components->ctx), 0), "");
        _apl_emit_arc_retain(components, ptr_val);
    }
    LLVMBuildRet(components->builder, ret_val);
  } else {
    LLVMBuildRetVoid(components->builder);
  }
}

/*==================================================FXN CALLS=====================================================*/

void _apl_gen_fxn_call_from_ast(LLVMComponents *components,
                                 ASTNode *stmt) {


  if (memcmp(stmt->call_fxn.name, "println", 7) == 0) {
    _apl_gen_println_ir(components, stmt->call_fxn.args);
    return;
  } else {

    _apl_eval_function_call(components, stmt);
    return;
  }
}
