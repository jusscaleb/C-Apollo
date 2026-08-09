#include "../../headers/llvm_backend.h"



void _apl_create_local_variable(LLVMComponents *components,
                                CodegenContext *context, ASTNode *var_node) {

  char var_name[var_node->var_decl.name_length + 1];
  memcpy(var_name, var_node->var_decl.name, var_node->var_decl.name_length);
  var_name[var_node->var_decl.name_length] = '\0';

  ASTNode *expr = var_node->var_decl.value;
  const DataType VAR_TYPE = var_node->var_decl.value_type;

  LLVMValueRef var_ptr;
  LLVMValueRef assign_val;
  LLVMValueRef var_val;

  switch (VAR_TYPE) {

  case TYPE_BOOL: {
    LLVMValueRef val;

    if (expr->Type == AST_LITERAL_EXPR) {
      const int LENGTH = expr->literal_expr.token.length;

      int b_val = (LENGTH == 5) ? 0 : 1;

      var_ptr =
          LLVMBuildAlloca(components->builder,
                          LLVMInt1TypeInContext(components->ctx), var_name);
      val = LLVMConstInt(LLVMInt1TypeInContext(components->ctx), b_val, 0);
    } else {
      val = arihmetics(components, expr, "");
    }

    LLVMBuildStore(components->builder, val, var_ptr);

    break;
  }
  case TYPE_FLOAT: {
    LLVMValueRef val;

    if (expr->Type == AST_LITERAL_EXPR) {
      float f_val = return_eval_int(expr);
      val = LLVMConstReal(LLVMFloatTypeInContext(components->ctx), f_val);
    } else {
      val = arihmetics(components, expr, "");
    }

    var_ptr = LLVMBuildAlloca(
        components->builder, LLVMFloatTypeInContext(components->ctx), var_name);
    LLVMBuildStore(components->builder, val, var_ptr);

    break;
  }
  case TYPE_INT: {
    LLVMValueRef val;
    if (expr->Type == AST_LITERAL_EXPR) {
      int i_val = (int)return_eval_int(expr);
      val = LLVMConstInt(LLVMInt32TypeInContext(components->ctx), i_val, 0);
    } else {
      val = arihmetics(components, expr, "");
    }

    var_ptr = LLVMBuildAlloca(
        components->builder, LLVMInt32TypeInContext(components->ctx), var_name);
    LLVMBuildStore(components->builder, val, var_ptr);
    break;
  }

  case TYPE_STRING: {
    char str[expr->literal_expr.token.length + 1];

    slice_string(expr->literal_expr.token, str);
    LLVMTypeRef str_members[] = {
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        LLVMInt32TypeInContext(components->ctx)};

    LLVMTypeRef string_struct_type =
        LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    var_ptr =
        LLVMBuildAlloca(components->builder, string_struct_type, var_name);
    LLVMTypeRef param_types[] = {
        LLVMPointerType(string_struct_type, 0),
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0)};
    LLVMTypeRef create_str_fxn_type = LLVMFunctionType(
        LLVMVoidTypeInContext(components->ctx), param_types, 2, false);
    LLVMValueRef create_str_fxn =
        LLVMGetNamedFunction(components->module, "__apl_create_str__");
    LLVMValueRef llvm_str_lit =
        LLVMBuildGlobalStringPtr(components->builder, str, "str_lit");
    LLVMValueRef args[] = {var_ptr, llvm_str_lit};

    LLVMValueRef ret_struct = LLVMBuildCall2(
        components->builder, create_str_fxn_type, create_str_fxn, args, 2, "");

    break;
  }
  default:
    break;
  }
  var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
}

void _apl_reassign_variable(LLVMComponents *components, CodegenContext *context,
                            ASTNode *node) {

  DataType var_Type = node->var_assign.resolved_symbol->type;
  LLVMValueRef var = node->var_assign.resolved_symbol->llvm_val_ref;
  LLVMValueRef new_val;
  ASTNode *value_node = node->var_assign.value;

  switch (var_Type) {
  case TYPE_INT: {
    if (value_node->Type == AST_LITERAL_EXPR) {
      int i_val = (int)return_eval_int(value_node);
      new_val = LLVMConstInt(LLVMInt32TypeInContext(components->ctx), i_val, 0);
    } else {
      new_val = arihmetics(components, value_node, "");
    }
    break;
  }

  case TYPE_FLOAT: {
    if (value_node->Type == AST_LITERAL_EXPR) {
      float f_val = return_eval_int(value_node);
      new_val = LLVMConstReal(LLVMFloatTypeInContext(components->ctx), f_val);
    } else {
      new_val = arihmetics(components, value_node, "");
    }
    break;
  }
  case TYPE_STRING: {
    break;
  }

  case TYPE_BOOL: {
    if (value_node->Type == AST_LITERAL_EXPR) {
      int b_val = (value_node->literal_expr.token.length == 5) ? 0 : 1;
      new_val = LLVMConstInt(LLVMInt1TypeInContext(components->ctx), b_val, 0);
    } else {
      new_val = arihmetics(components, value_node, "");
    }
    break;
  }
  default:
    break;
  }
  LLVMBuildStore(components->builder, new_val, var);
}
