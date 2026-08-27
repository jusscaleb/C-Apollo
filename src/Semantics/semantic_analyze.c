#include "../../headers/semantic.h"

// Main entry point
void analyze_semantics(SemanticContext *context, ASTNode *node) {
  if (!node)
    return;

  analyze_node(context, node);
}

static void analyze_node(SemanticContext *context, ASTNode *node) {

  if (!node)
    return;

  switch (node->Type) {
  case AST_PROGRAM:
    if (node->program.function) {
      analyze_node(context, node->program.function);
    }
    break;

  case AST_FUNCTION:

    if (node->function.fxn.return_type != TYPE_NULL &&
        (node->function.fxn.block_nodes != node->function.fxn.ret_nodes)) {
      if (node->function.fxn.block_nodes > node->function.fxn.ret_nodes)
        report_semantic_error(context,
                              "All branches of fxn should return a value.");
      else
        report_semantic_error(context, "Expression cannot be reached in fxn.");
    }

    if (lookup_token(context->codegen, node->function.name, &node->function.fxn,
                     node->function.level)) {
      report_semantic_error(context, "Cannot redefine function.");
      break;
    } else {
      node->function.resolved_symbol =
          register_fxn(context->codegen, node->function.name,
                       node->function.fxn.return_type, node->function.level,
                       &node->function.fxn, node->function.name_length);
      node->function.resolved_symbol->pointer_level =
          node->function.pointer_level;
    }

    // Analyze Parameters.
    Params *param = node->function.fxn.params;
    int p_idx = 0;
    while (param) {
      analyze_node(context, param->param);
      if (param->param->var_decl.resolved_symbol) {
        param->param->var_decl.resolved_symbol->param_idx = p_idx;
      }
      p_idx++;
      param = param->next;
    }

    // Analyze Function Body.
    if (node->function.body) {
      analyze_node(context, node->function.body);
    }

    break;

  case AST_BLOCK:
    analyze_block(context, node);
    break;

  case AST_VAR_DECL: {
    const int NAME_LENGTH = node->var_decl.name_length;
    char name[NAME_LENGTH + 1];
    memcpy(name, node->var_decl.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';
    Symbol *sym = lookup_token(context->codegen, name, &node->var_decl.fxn,
                               node->var_decl.level);
    if (sym && memcmp(sym->fxn->name, node->var_decl.fxn.name,
                      strlen(sym->fxn->name)) == 1) {
      report_semantic_error(context, "Multiple definition of variable.");
      break;
    }

    DataType inferred;
    if (node->var_decl.value) {
      analyze_node(context, node->var_decl.value);
      if (node->var_decl.value_type == TYPE_NULL) {
        inferred = infer_expr_type(context, node->var_decl.value);
        node->var_decl.value_type = inferred;

      } else {
        DataType expected_type = node->var_decl.value_type;
        inferred = infer_expr_type(context, node->var_decl.value);
        int inferred_ptr_level =
            infer_expr_pointer_level(context, node->var_decl.value);
        MemoryBucket val_bucket =
            infer_bucket_type(context, node->var_decl.value);
        MemoryBucket decl_bucket = node->var_decl.bucket;

        bool is_ptr_compatible =
            (inferred_ptr_level == node->var_decl.pointer_level) ||
            (node->var_decl.value->Type == AST_LITERAL_EXPR &&
             node->var_decl.pointer_level > 0);

        bool is_compatible =
            ((inferred == expected_type) ||
             (inferred == TYPE_INT && expected_type == TYPE_CHAR) ||
             (inferred == TYPE_CHAR && expected_type == TYPE_INT)) &&
            is_ptr_compatible;

        if (!is_compatible) {
          report_semantic_error(context, "Datatype Mismatch.");
          break;
        }
      }

      node->var_decl.resolved_symbol = register_variable(
          context->codegen, name, inferred, &node->var_decl.fxn,
          node->var_decl.level, NAME_LENGTH);

      node->var_decl.resolved_symbol->pointer_level =
          node->var_decl.pointer_level;
      node->var_decl.resolved_symbol->bucket = node->var_decl.bucket;

    } else {
      node->var_decl.value_type = TYPE_NULL;
      node->var_decl.resolved_symbol = register_variable(
          context->codegen, name, TYPE_NULL, &node->var_decl.fxn,
          node->var_decl.level, NAME_LENGTH);
    }
    break;
  }

  case AST_VAR_ASS: {
    const int NAME_LENGTH = node->var_assign.name_length;
    char name[NAME_LENGTH + 1];
    memcpy(name, node->var_assign.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_assign.fxn,
                               node->var_assign.level);
    if (!sym) {
      report_semantic_error(context, "Assignment to undeclared variable.");
      break;
    } else {
      node->var_assign.resolved_symbol = sym;
      if(node->var_assign.resolved_symbol->bucket == BUCKET_ONE){
        report_semantic_error(context, "Cannot reassign constant variable.");
        break;
      }

      if (node->var_assign.value) {
        analyze_node(context, node->var_assign.value);
        DataType inferred = infer_expr_type(context, node->var_assign.value);
        int inferred_ptr_level =
            infer_expr_pointer_level(context, node->var_assign.value);
        int target_ptr_level =
            sym->pointer_level - node->var_assign.deref_level;
        MemoryBucket inferred_bucket_type =
            infer_bucket_type(context, node->var_assign.value);
        MemoryBucket target_bucket_type = sym->bucket;

        if (target_ptr_level < 0) {
          report_semantic_error(context,
                                "Cannot dereference non-pointer variable.");
          break;
        }

        if (sym->type != TYPE_NULL && inferred != TYPE_NULL &&
            (sym->type != inferred || target_ptr_level != inferred_ptr_level)) {
          report_semantic_error(context, "Incompatible assignment type.");
          break;
        } else if (sym->type == TYPE_NULL && inferred != TYPE_NULL) {
          sym->type = inferred;
          sym->pointer_level = target_ptr_level;
        }
      }
    }
    break;
  }

  case AST_IF: {
    ASTNode *cond = node->if_stmt.condition;
    if (infer_expr_type(context, cond) != TYPE_BOOL) {
      report_semantic_error(context,
                            "Expected a boolean expression or literal.");
      break;
    }
    analyze_node(context, node->if_stmt.condition);
    analyze_node(context, node->if_stmt.then_block);
    if (node->if_stmt.else_block) {
      analyze_node(context, node->if_stmt.else_block);
    }
    break;
  }
  case AST_WHILE: {
    ASTNode *cond = node->while_lp.condition;
    if (infer_expr_type(context, cond) != TYPE_BOOL) {
      report_semantic_error(context,
                            "Expected a boolean expression or literal.");
      break;
    }
    analyze_node(context, node->while_lp.condition);
    analyze_node(context, node->while_lp.then_block);
    break;
  }
  case AST_FOR:
    ASTNode *cond = node->for_lp.condtion;
    ASTNode *op = node->for_lp.var_operation;

    bool case_1 = cond->Type != AST_BINARY_EXPR;
    bool case_2 = infer_expr_type(context, cond) != TYPE_BOOL;

    if (case_1 || case_2) {
      report_semantic_error(
          context, "Expected a boolean binary expression: E.g.: 'x > 5'");
      break;
    }

    analyze_node(context, node->for_lp.variable);
    analyze_node(context, node->for_lp.condtion);
    analyze_node(context, node->for_lp.var_operation);
    analyze_node(context, node->for_lp.then_block);
    break;

  case AST_BINARY_EXPR: {
    ASTNode *left = node->binary_expr.left;
    ASTNode *right = node->binary_expr.right;

    analyze_node(context, left);
    analyze_node(context, right);

    DataType l_type = infer_expr_type(context, left),
             r_type = infer_expr_type(context, right);
    if (l_type != r_type) {
      if ((l_type == TYPE_INT && r_type != TYPE_FLOAT) ||
          (l_type == TYPE_BOOL || r_type == TYPE_BOOL)) {
        report_semantic_error(
            context, "Binary expression contains incompatible operands.");
        break;
      }
    }
    break;
  }

  case AST_RET_NODE: {
    if (node->ret_node.value) {
      analyze_node(context, node->ret_node.value);
    }

    DataType fxn_ret_type = node->ret_node.fxn.return_type;
    DataType ret_type = infer_expr_type(context, node->ret_node.value);
    MemoryBucket ret_bucket_type =
        infer_bucket_type(context, node->ret_node.value);
    MemoryBucket fxn_bucket_type = node->ret_node.fxn.bucket;

    bool is_type_compatible =
        (ret_type == fxn_ret_type) ||
        (ret_type == TYPE_CHAR && fxn_ret_type == TYPE_INT) ||
        (ret_type == TYPE_INT && fxn_ret_type == TYPE_CHAR);

    bool is_bucket_compatible = false;
    if (fxn_bucket_type == BUCKET_THREE) {
      bool is_ret_literal = (node->ret_node.value->Type == AST_LITERAL_EXPR);
      is_bucket_compatible =
          (ret_bucket_type == BUCKET_THREE) || is_ret_literal;
    } else {
      is_bucket_compatible = (ret_bucket_type == fxn_bucket_type) ||
                             (ret_bucket_type == BUCKET_PLUS_ONE) ||
                             (fxn_bucket_type == BUCKET_PLUS_ONE);
    }

    bool is_compatible = is_type_compatible && is_bucket_compatible;

    if (node->ret_node.value) {
      if (fxn_ret_type == TYPE_NULL) {
        report_semantic_error(
            context, "Fxn of return type \"null\" cannot return value.");
        break;
      }
      if (!is_compatible) {
        report_semantic_error(context,
                              "Return value for function doesn't match.");
        break;
      }
    } else {
      report_semantic_error(context, "Value not provided.");
      break;
    }
    break;
  }

  case AST_CALL_FXN: {
    const int NAME_LENGTH = node->call_fxn.name_length;
    char fn_name[NAME_LENGTH + 1];
    memcpy(fn_name, node->call_fxn.name, NAME_LENGTH);
    fn_name[NAME_LENGTH] = '\0';

    bool is_println = (memcmp(fn_name, "println", 7) == 0);
    Symbol *sym = NULL;

    if (is_println) {
      node->call_fxn.return_type = TYPE_NULL;
    } else {
      sym = lookup_token(context->codegen, fn_name, &node->call_fxn.fxn,
                         node->call_fxn.level);
      if (!sym) {
        report_semantic_error(context, "Call to undeclared function.");
        break;
      }
      node->call_fxn.return_type = sym->type;
      node->call_fxn.resolved_symbol = sym;
    }

    Params *expected_param = (sym && sym->fxn) ? sym->fxn->params : NULL;
    Args *passed_arg = node->call_fxn.args;
    bool passed_arg_ = passed_arg == NULL;
    bool expected_param_ = expected_param == NULL;

    if (passed_arg == NULL && expected_param != NULL && !is_println) {
      report_semantic_error(context, "Too few arguments in fxn call.");
      break;
    }
    if (passed_arg != NULL && expected_param == NULL && !is_println) {
      report_semantic_error(context, "Too many arguments in fxn call.");
      break;
    }

    while (passed_arg != NULL) {
      analyze_node(context, passed_arg->arg);
      passed_arg->datatype = infer_expr_type(context, passed_arg->arg);

      if (expected_param != NULL) {
        int passed_ptr_level =
            infer_expr_pointer_level(context, passed_arg->arg);
        MemoryBucket passed_bucket =
            infer_bucket_type(context, passed_arg->arg);
        MemoryBucket expected_bucket = expected_param->param->var_decl.bucket;

        bool is_passed_literal = (passed_arg->arg->Type == AST_LITERAL_EXPR);
        bool bucket_ok = false;

        if (expected_bucket == BUCKET_THREE) {
          // Parameter expects Heap object (@) -> Argument must be Heap object
          // or literal constant
          bucket_ok = (passed_bucket == BUCKET_THREE) || is_passed_literal;
        } else if (passed_bucket == BUCKET_THREE) {
          // Parameter expects Stack -> Passing Heap object (@) without
          // dereferencing is invalid
          bucket_ok = false;
        } else {
          bucket_ok = (passed_bucket == expected_bucket) ||
                      (passed_bucket == BUCKET_PLUS_ONE) ||
                      (expected_bucket == BUCKET_PLUS_ONE);
        }

        if (passed_arg->datatype !=
                expected_param->param->var_decl.value_type ||
            passed_ptr_level != expected_param->param->var_decl.pointer_level ||
            !bucket_ok) {
          report_semantic_error(
              context,
              "Argument type or memory bucket mismatch in function call.");
          break;
        }
        expected_param = expected_param->next;
      }

      passed_arg = passed_arg->next;

      if (is_println)
        continue;
      if (passed_arg == NULL && expected_param != NULL) {
        report_semantic_error(context, "Too many arguments in fxn call.");
        break;
      }

      if (passed_arg != NULL && expected_param == NULL) {
        report_semantic_error(context, "Too few arguments in fxn call.");
        break;
      }
    }

    break;
  }

  case AST_LITERAL_EXPR:
    break;

  case AST_VAR_REF: {
    const int NAME_LENGTH = node->var_ref.name_length;
    char name[NAME_LENGTH + 1];
    memcpy(name, node->var_ref.name, NAME_LENGTH);
    name[NAME_LENGTH] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_ref.fxn,
                               node->var_ref.level);
    if (!sym) {
      report_semantic_error(
          context, "Referenced Variable not found in the current scope.");
      break;
    } else {
      node->var_ref.resolved_symbol = sym;
    }
    break;
  }
  case AST_URINARY_EXPR: {
    TokenType op_type = node->urinary_expr.operator_type;

    switch (op_type) {
    case TOKEN_REF: {
      analyze_node(context, node->urinary_expr.value);

      if (node->urinary_expr.value->Type == AST_VAR_REF) {
        node->urinary_expr.eval_type = infer_expr_type(context, node);
      } else {
        report_semantic_error(context,
                              "Cannot reference a non-variable literal.");
        break;
      }
      break;
    }
    case TOKEN_MUL:
    case TOKEN_SUB: {
      analyze_node(context, node->urinary_expr.value);
      node->urinary_expr.eval_type = infer_expr_type(context, node);
      break;
    }
    default:
      break;
    }
    break;
  }
  }
}

static void analyze_block(SemanticContext *context, ASTNode *block_node) {
  if (!block_node || block_node->Type != AST_BLOCK)
    return;

  int initial_symbol_count = context->codegen->symbol_count;

  for (int i = 0; i < block_node->block.count; ++i) {
    analyze_node(context, block_node->block.statements[i]);
  }

  for (int i = initial_symbol_count; i < context->codegen->symbol_count; ++i) {
    context->codegen->symbols[i].is_active = false;
  }
}
