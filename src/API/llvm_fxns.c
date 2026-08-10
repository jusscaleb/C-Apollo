#include "../../headers/llvm_backend.h"
#include <stdint.h>
#include <string.h>

/*================================================== INBUILT FUNCTIONS
 * =====================================================*/
void _apl_gen_println_ir(LLVMComponents *components, CodegenContext *context,
                         Args *args) {

  DataType arg_type = args->datatype;
  ASTNode *expr = args->arg;
  LLVMTypeRef param_types[1];
  LLVMValueRef println_fxn, println_args;
  LLVMTypeRef func_type;

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
    if (expr->Type == AST_BINARY_EXPR) {
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
    param_types[0] = LLVMInt1TypeInContext(components->ctx);
    println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_bool");

    if (expr->Type == AST_VAR_REF) {
      println_args = load_variable(components, expr);
      break;
    }

    int b;
    const int LENGTH = expr->literal_expr.token.length;

    b = (LENGTH == 5) ? 0 : 1;
    println_args = LLVMConstInt(param_types[0], b, 0);
    break;

  case TYPE_CHAR:
    break;
  case TYPE_STRING: {
    param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
    println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_string");

    if (expr->Type == AST_VAR_REF) {
      println_args = load_variable(components, expr);
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
    char null_str[4] = "null";

    break;

  default:
    println_fxn =
        LLVMGetNamedFunction(components->module, "_apl_print_newline");
    break;
  }

  func_type = LLVMFunctionType(LLVMVoidTypeInContext(components->ctx),
                               param_types, 1, 0);

  LLVMBuildCall2(components->builder, func_type, println_fxn, &println_args, 1,
                 "");

  if (args->next) {
    _apl_gen_println_ir(components, context, args->next);
  }

  LLVMValueRef newline_fxn =
      LLVMGetNamedFunction(components->module, "_apl_print_newline");
  LLVMTypeRef newline_type =
      LLVMFunctionType(LLVMVoidTypeInContext(components->ctx), NULL, 0, false);
  LLVMBuildCall2(components->builder, newline_type, newline_fxn, NULL, 0, "");

  return;
}

void _apl_gen_function_start(LLVMComponents *components,
                             CodegenContext *context, ASTNode *block_node) {
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMTypeRef main_fxn_type = LLVMFunctionType(
        LLVMInt32TypeInContext(components->ctx), NULL, 0, false);
    LLVMValueRef main_fxn =
        LLVMAddFunction(components->module, "main", main_fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, main_fxn, "");
    LLVMPositionBuilderAtEnd(components->builder, entry);

    components->current_fxn = main_fxn;
    return;
  }

  else {
    uint32_t p_number =
        (block_node->function.fxn.params)
            ? _apl_get_n_params(components, block_node->function.fxn.params)
            : 0;
    LLVMTypeRef param_types[p_number];

    Params *p = block_node->function.fxn.params;

    for (uint32_t i = 0; i < p_number; i++) {

      /*switch (p->param->var_decl.value_type) {
      case TYPE_INT: {
        param_types[i] = LLVMInt32TypeInContext(components->ctx);
        break;
      }case TYPE_FLOAT:{
        param_types[i] = LLVMFloatTypeInContext(context->ctx);
      }
      }*/

      param_types[i] = _apl_get_llvm_type(components, p->param->var_decl.value_type);

      p = p->next;
    }

    LLVMTypeRef fxn_type = LLVMFunctionType(
        _enquire_fxn_return_type(components, &block_node->function.fxn),
        (p_number == 0) ? NULL : param_types, p_number, false);
    LLVMValueRef fxn = LLVMAddFunction(components->module,
                                       block_node->function.name, fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, fxn, "");
    LLVMPositionBuilderAtEnd(components->builder, entry);

    block_node->function.resolved_symbol->fxn_meta_data.fxn_type = fxn_type;
    block_node->function.resolved_symbol->fxn_meta_data.the_fxn = fxn;
    components->current_fxn = fxn;

    p = block_node->function.fxn.params;
    for (uint32_t i = 0; i < p_number; i++) {
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
    return;
  }
}

void _apl_gen_function_end(LLVMComponents *components, CodegenContext *context,
                           ASTNode *block_node) {

  LLVMBasicBlockRef current_block = LLVMGetInsertBlock(components->builder);

  if (LLVMGetBasicBlockTerminator(current_block)) return;
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMBuildRet(
        components->builder,
        LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false));
  } else {
    LLVMBuildRetVoid(components->builder);
  }
}

void _apl_gen_return(LLVMComponents *components, CodegenContext *context,
                     ASTNode *node) {
  LLVMValueRef ret_val;
  switch (node->ret_node.fxn.return_type) {
  case TYPE_INT: {
    if (node->ret_node.value->Type == AST_BINARY_EXPR) {
      ret_val = arihmetics(components, node->ret_node.value, "");
      break;
    }
    if (node->ret_node.value->Type == AST_VAR_REF) {
      ret_val = load_variable(components, node->ret_node.value);
      break;
    }
    if (node->ret_node.value->Type == AST_CALL_FXN) {
      ret_val = _apl_eval_function_call(components, node->ret_node.value);
      break;
    }
    uint32_t number = (int)return_eval_int(node->ret_node.value);
    ret_val =
        LLVMConstInt(LLVMInt32TypeInContext(components->ctx), number, false);
    break;
  }
  case TYPE_BOOL: {
    uint8_t b = (node->ret_node.value->literal_expr.token.length >= 4) ? 1 : 0;
    ret_val = LLVMConstInt(LLVMInt1TypeInContext(components->ctx), b, false);
    break;
  }
  case TYPE_STRING: {
    const uint32_t LENGTH = node->ret_node.value->literal_expr.token.length;
    char str[LENGTH + 1];
    slice_string(node->ret_node.value->literal_expr.token, str);
    ret_val = LLVMBuildGlobalStringPtr(components->builder, str, "");
    break;
  }
  case TYPE_FLOAT: {
    float number = return_eval_int(node->ret_node.value);
    ret_val = LLVMConstReal(LLVMFloatTypeInContext(components->ctx), number);
  }
  }

  LLVMBuildRet(components->builder, ret_val);
}

/*==================================================FXN
 * CALLS=====================================================*/

void _apl_gen_fxn_call_from_ast(LLVMComponents *components,
                                CodegenContext *context, ASTNode *stmt) {


  if (memcmp(stmt->call_fxn.name, "println", 7) == 0) {
    _apl_gen_println_ir(components, context, stmt->call_fxn.args);
    return;
  } else {

    _apl_eval_function_call(components, stmt);
    return;
  }
}
