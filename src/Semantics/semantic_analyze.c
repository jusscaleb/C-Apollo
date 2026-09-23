#include "../../headers/semantic.h"
#include <stdint.h>

// Main entry point
void analyze_semantics(SemanticContext *context, ASTNode *node) {
  if (!node)
    return;

  analyze_node(context, node);
}

static void analyze_node(SemanticContext *context, ASTNode *node) {

  if (!node)
    return;

  ASTNode *prev_node = context->current_node;
  context->current_node = node;

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
      node->function.resolved_symbol->bucket = node->function.fxn.bucket;
    }

    // Analyze Parameters.
    Params *param = node->function.fxn.params;
    int p_idx = 0;
    while (param) {
      analyze_node(context, param->param);
      if (param->param->var_decl.resolved_symbol) {
        param->param->var_decl.resolved_symbol->param_idx = p_idx;
        param->param->var_decl.resolved_symbol->assigned = true;
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
    char name[256];
    int len_d = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(name, node->var_decl.name, len_d);
    name[len_d] = '\0';
    Symbol *sym = lookup_token(context->codegen, name, &node->var_decl.fxn,
                               node->var_decl.level);
    if (sym && sym->fxn && sym->fxn->name && node->var_decl.fxn.name &&
        strcmp(sym->fxn->name, node->var_decl.fxn.name) == 0 &&
        sym->scope_level == node->var_decl.level) {
      report_semantic_error(context, "Multiple definition of variable.");
      break;
    }
    


    Symbol *struct_sym = NULL;
    if (node->var_decl.value_type == TYPE_STRUCT && node->var_decl.struct_type_name) {
      struct_sym = lookup_token(context->codegen, node->var_decl.struct_type_name,
                                &node->var_decl.fxn, node->var_decl.level);
      if (!struct_sym || struct_sym->type != TYPE_STRUCT) {
        report_semantic_error(context, "Undefined struct type.");
        break;
      }

      if (node->var_decl.value) {
        if (node->var_decl.value->Type == AST_ARRAY_LITERAL) {
          ASTNode **elements = node->var_decl.value->array_literal.elements;
          uint32_t count = node->var_decl.value->array_literal.count;

          uint32_t expected_count = 0;
          Struct *f = struct_sym->struct_fields;
          while (f) {
            if (f->field) expected_count++;
            f = f->next;
          }

          if (count != expected_count) {
            report_semantic_error(context, "Struct initializer field count mismatch.");
            break;
          }

          f = struct_sym->struct_fields;
          for (uint32_t i = 0; i < count && f; i++) {
            if (elements[i]) {
              analyze_node(context, elements[i]);
              DataType elem_type = infer_expr_type(context, elements[i]);
              int elem_ptr = infer_expr_pointer_level(context, elements[i]);
              bool is_lit = (elements[i]->Type == AST_LITERAL_EXPR);

              DataType field_type = f->field->var_decl.value_type;
              int field_ptr = f->field->var_decl.pointer_level;

              if (!is_types_compatible(field_type, elem_type, field_ptr, elem_ptr, is_lit)) {
                report_semantic_error(context, "Datatype mismatch in struct field initializer.");
                break;
              }
            }
            f = f->next;
          }
        }
      }
    } else if (node->var_decl.value) {
      analyze_node(context, node->var_decl.value);
      if (node->var_decl.value_type == TYPE_NULL) {
        DataType inferred = infer_expr_type(context, node->var_decl.value);
        node->var_decl.value_type = inferred;
      } else {
        DataType expected_type = node->var_decl.value_type;
        DataType inferred = infer_expr_type(context, node->var_decl.value);
        int inferred_ptr_level =
            infer_expr_pointer_level(context, node->var_decl.value);
        bool is_literal = (node->var_decl.value->Type == AST_LITERAL_EXPR);
        bool is_compatible = is_types_compatible(
            expected_type, inferred, node->var_decl.pointer_level,
            inferred_ptr_level, is_literal);

        if (!is_compatible) {
          report_semantic_error(context, "Datatype Mismatch.");
          break;
        }
      }
    }

    node->var_decl.resolved_symbol = register_variable(
        context->codegen, name, node->var_decl.value_type,
        &node->var_decl.fxn, node->var_decl.level, NAME_LENGTH);

    node->var_decl.resolved_symbol->pointer_level =
        node->var_decl.pointer_level;
    node->var_decl.resolved_symbol->bucket = node->var_decl.bucket;
    node->var_decl.resolved_symbol->array_count = node->var_decl.array_count;
    node->var_decl.resolved_symbol->assigned = (node->var_decl.value || node->var_decl.value_type == TYPE_STRUCT) ? true : false;
    if (struct_sym) {
      node->var_decl.resolved_symbol->llvm_struct_type = struct_sym->llvm_struct_type;
      node->var_decl.resolved_symbol->struct_fields = struct_sym->struct_fields;
      node->var_decl.resolved_symbol->struct_type_name = (char *)node->var_decl.struct_type_name;
    }
    break;
  }

  case AST_VAR_ASS: {
    if (node->var_assign.target_node != NULL) {
      analyze_node(context, node->var_assign.target_node);
      if (node->var_assign.value) {
        analyze_node(context, node->var_assign.value);
        DataType target_type = infer_expr_type(context, node->var_assign.target_node);
        DataType value_type = infer_expr_type(context, node->var_assign.value);
        int target_ptr = infer_expr_pointer_level(context, node->var_assign.target_node);
        int val_ptr = infer_expr_pointer_level(context, node->var_assign.value);
        bool is_lit = (node->var_assign.value->Type == AST_LITERAL_EXPR);

        if (!is_types_compatible(target_type, value_type, target_ptr, val_ptr, is_lit)) {
          report_semantic_error(context, "Datatype mismatch in member assignment.");
        }
      }
      break;
    }

    const int NAME_LENGTH = node->var_assign.name_length;
    char name[256];
    int len_name = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(name, node->var_assign.name, len_name);
    name[len_name] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_assign.fxn,
                               node->var_assign.level);
    if (!sym) {
      report_semantic_error(context, "Assignment to undeclared variable.");
      break;
    } else {
      node->var_assign.resolved_symbol = sym;
      if (node->var_assign.resolved_symbol->bucket == BUCKET_ONE) {
        report_semantic_error(context, "Cannot reassign constant variable.");
        break;
      }

      if (node->var_assign.value) {
        if (sym->type == TYPE_STRUCT && node->var_assign.value->Type == AST_ARRAY_LITERAL) {
          ASTNode **elements = node->var_assign.value->array_literal.elements;
          uint32_t count = node->var_assign.value->array_literal.count;

          uint32_t expected_count = 0;
          Struct *f = sym->struct_fields;
          while (f) {
            if (f->field) expected_count++;
            f = f->next;
          }

          if (count != expected_count) {
            report_semantic_error(context, "Struct initializer field count mismatch.");
            break;
          }

          f = sym->struct_fields;
          for (uint32_t i = 0; i < count && f; i++) {
            if (elements[i]) {
              analyze_node(context, elements[i]);
              DataType elem_type = infer_expr_type(context, elements[i]);
              int elem_ptr = infer_expr_pointer_level(context, elements[i]);
              bool is_lit = (elements[i]->Type == AST_LITERAL_EXPR);

              DataType field_type = f->field->var_decl.value_type;
              int field_ptr = f->field->var_decl.pointer_level;

              if (!is_types_compatible(field_type, elem_type, field_ptr, elem_ptr, is_lit)) {
                report_semantic_error(context, "Datatype mismatch in struct field initializer.");
                break;
              }
            }
            f = f->next;
          }
        } else {
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

          bool is_literal = (node->var_assign.value->Type == AST_LITERAL_EXPR);
          if (sym->type != TYPE_NULL && inferred != TYPE_NULL &&
              !is_types_compatible(sym->type, inferred, target_ptr_level,
                                   inferred_ptr_level, is_literal)) {
            report_semantic_error(context, "Incompatible assignment type.");
            break;
          } else if (sym->type == TYPE_NULL && inferred != TYPE_NULL) {
            sym->type = inferred;
            sym->pointer_level = target_ptr_level;
          }
        }
      }
    }
    node->var_assign.resolved_symbol->assigned = true;
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
    char fn_name[256];
    int len_fn = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(fn_name, node->call_fxn.name, len_fn);
    fn_name[len_fn] = '\0';

    bool is_println = (memcmp(fn_name, "println", 7) == 0);
    Symbol *sym = NULL;

    if (is_println) {
      node->call_fxn.return_type = TYPE_NULL;
    } else {
      sym = lookup_token(context->codegen, fn_name, &node->call_fxn.fxn,
                         node->call_fxn.level);
      if (!sym) {
        char *underscore = strchr(fn_name, '_');
        if (underscore) {
          int rec_len = (int)(underscore - fn_name);
          char rec_name[256];
          if (rec_len >= 256) rec_len = 255;
          memcpy(rec_name, fn_name, rec_len);
          rec_name[rec_len] = '\0';
          Symbol *rec_sym = lookup_token(context->codegen, rec_name, &node->call_fxn.fxn, node->call_fxn.level);
          if (rec_sym && (rec_sym->type == TYPE_STRUCT || rec_sym->struct_fields)) {
            const char *struct_name = rec_sym->struct_type_name ? rec_sym->struct_type_name : rec_sym->name;
            const char *method_name = underscore + 1;
            char mangled[256];
            snprintf(mangled, sizeof(mangled), "%s_%s", struct_name, method_name);
            sym = lookup_token(context->codegen, mangled, &node->call_fxn.fxn, node->call_fxn.level);
            if (sym) {
              Params *fp = (sym->fxn) ? sym->fxn->params : NULL;
              if (fp && fp->param && strcmp(fp->param->var_decl.name, "self") == 0) {
                ASTNode *rec_node = create_var_ref_node(rec_sym->name, rec_sym->name_length, node->call_fxn.fxn, node->call_fxn.level, context->codegen->a);
                rec_node->var_ref.resolved_symbol = rec_sym;
                Args *this_arg = (Args *)arena_alloc(context->codegen->a, sizeof(Args));
                this_arg->arg = rec_node;
                this_arg->datatype = TYPE_STRUCT;
                this_arg->next = node->call_fxn.args;
                node->call_fxn.args = this_arg;
              }
              int mangled_len = strlen(mangled);
              char *mangled_dup = (char *)arena_alloc(context->codegen->a, mangled_len + 1);
              memcpy(mangled_dup, mangled, mangled_len);
              mangled_dup[mangled_len] = '\0';
              node->call_fxn.name = mangled_dup;
              node->call_fxn.name_length = mangled_len;
            }
          }
        }

        if (!sym) {
          for (int s_idx = 0; s_idx < context->codegen->symbol_count; s_idx++) {
            Symbol *s = &context->codegen->symbols[s_idx];
            if (s->is_active && s->t_type == FUNC && s->name) {
              const char *m_suffix = strchr(s->name, '_');
              if (m_suffix && strcmp(m_suffix + 1, fn_name) == 0) {
                sym = s;
                node->call_fxn.name = s->name;
                node->call_fxn.name_length = s->name_length;
                break;
              }
            }
          }
        }

        if (!sym) {
          report_semantic_error(context, "Call to undeclared function.");
          break;
        }
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
          bucket_ok = (passed_bucket == BUCKET_THREE) || is_passed_literal;
        } else if (passed_bucket == BUCKET_THREE) {
          bucket_ok = false;
        } else {
          bucket_ok = (passed_bucket == expected_bucket) ||
                      (passed_bucket == BUCKET_PLUS_ONE) ||
                      (expected_bucket == BUCKET_PLUS_ONE);
        }

        bool is_this_param = (expected_param->param && expected_param->param->var_decl.value_type == TYPE_STRUCT &&
                              expected_param->param->var_decl.pointer_level > 0 &&
                              passed_arg->datatype == TYPE_STRUCT);

        if (!is_this_param && (passed_arg->datatype !=
                expected_param->param->var_decl.value_type ||
            passed_ptr_level != expected_param->param->var_decl.pointer_level ||
            !bucket_ok)) {
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
        report_semantic_error(context, "Too few arguments in fxn call.");
        break;
      }

      if (passed_arg != NULL && expected_param == NULL) {
        report_semantic_error(context, "Too many arguments in fxn call.");
        break;
      }
    }

    break;
  }

  case AST_LITERAL_EXPR:
    break;

  case AST_VAR_REF: {

    const int NAME_LENGTH = node->var_ref.name_length;
    char name[256];
    int len_ref = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(name, node->var_ref.name, len_ref);
    name[len_ref] = '\0';

    Symbol *sym = lookup_token(context->codegen, name, &node->var_ref.fxn,
                               node->var_ref.level);
    if (!sym) {
      report_semantic_error(
          context, "Referenced Variable not found in the current scope.");
      break;
    } else {
      node->var_ref.resolved_symbol = sym;
    }

    if(!node->var_ref.resolved_symbol->assigned){
      report_semantic_error(context, "Use of unassigned variable.");
      break;
    }

    break;
  }
  case AST_URINARY_EXPR: {
    TokenType op_type = node->urinary_expr.operator_type;

    switch (op_type) {
    case TOKEN_REF: {
      analyze_node(context, node->urinary_expr.value);

      ASTNode *val_node = node->urinary_expr.value;
      bool is_valid_lvalue =
          (val_node->Type == AST_VAR_REF || val_node->Type == AST_INDEX_EXPR ||
           val_node->Type == AST_ACCESS ||
           (val_node->Type == AST_URINARY_EXPR &&
            val_node->urinary_expr.operator_type == TOKEN_MUL));
      if (is_valid_lvalue) {
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
      if (op_type == TOKEN_MUL) {
        uint32_t p_level =
            infer_expr_pointer_level(context, node->urinary_expr.value);
        if (p_level == 0) {
          report_semantic_error(context, "Operand must be a pointer.");
          break;
        }
      }
      break;
    }
    default:
      break;
    }
    break;
  }
  case AST_ARRAY_LITERAL: {
    if (node->array_literal.element_type == TYPE_STRUCT) {
      break;
    }
    ASTNode **elements = node->array_literal.elements;

    if (node->array_literal.element_type == TYPE_NULL &&
        node->array_literal.count > 0 && elements[0]) {
      node->array_literal.element_type = infer_expr_type(context, elements[0]);
    }

    if (!node->array_literal.is_dynamic)
      if (node->array_literal.count != node->array_literal.capacity) {
        report_semantic_error(
            context,
            "The number of elements do not match the declared length.");
        break;
      }

    for (uint32_t i = 0; i < node->array_literal.count; i++) {
      if (!elements[i])
        continue;

      analyze_node(context, elements[i]);

      DataType elem_type = infer_expr_type(context, elements[i]);
      int elem_ptr_level = infer_expr_pointer_level(context, elements[i]);
      bool is_lit = (elements[i]->Type == AST_LITERAL_EXPR);

      if (!is_types_compatible(node->array_literal.element_type, elem_type, 0,
                               elem_ptr_level, is_lit)) {
        report_semantic_error(context, "Array element type mismatch.");
        break;
      }
    }
    break;
  }

  case AST_INDEX_EXPR: {
    analyze_node(context, node->index_expr.target);
    analyze_node(context, node->index_expr.index);

    DataType idx_type = infer_expr_type(context, node->index_expr.index);
    if (idx_type != TYPE_INT) {
      report_semantic_error(context,
                            "Array index must evaluate to an integer.");
    }

    node->index_expr.type = infer_expr_type(context, node);
    if (node->index_expr.type == TYPE_NULL) {
      report_semantic_error(context, "Indexing target must be an array type.");
    }
    break;
  }

  case AST_ACCESS: {
    analyze_node(context, node->access.src);
    if (node->access.target) {
      if (node->access.target->Type == AST_CALL_FXN) {
        Symbol *src_sym = NULL;
        if (node->access.src->Type == AST_VAR_REF) {
          src_sym = node->access.src->var_ref.resolved_symbol;
        }
        const char *struct_name = (src_sym && src_sym->struct_type_name) ? src_sym->struct_type_name : (src_sym ? src_sym->name : NULL);
        if (struct_name) {
          char mangled[256];
          snprintf(mangled, sizeof(mangled), "%s_%s", struct_name, node->access.target->call_fxn.name);
          Symbol *sym = lookup_token(context->codegen, mangled, &node->access.target->call_fxn.fxn, node->access.target->call_fxn.level);
          if (sym) {
            Params *fp = (sym->fxn) ? sym->fxn->params : NULL;
            if (fp && fp->param && strcmp(fp->param->var_decl.name, "self") == 0) {
              Args *this_arg = (Args *)arena_alloc(context->codegen->a, sizeof(Args));
              this_arg->arg = node->access.src;
              this_arg->datatype = TYPE_STRUCT;
              this_arg->next = node->access.target->call_fxn.args;
              node->access.target->call_fxn.args = this_arg;
            }
            int mangled_len = strlen(mangled);
            char *mangled_dup = (char *)arena_alloc(context->codegen->a, mangled_len + 1);
            memcpy(mangled_dup, mangled, mangled_len);
            mangled_dup[mangled_len] = '\0';
            node->access.target->call_fxn.name = mangled_dup;
            node->access.target->call_fxn.name_length = mangled_len;
          } else {
            report_semantic_error(context, "Method not found on struct.");
            break;
          }
        }
        analyze_node(context, node->access.target);
        node->access.datatype = node->access.target->call_fxn.return_type;
      } else {
        node->access.datatype = infer_expr_type(context, node);
        node->access.fields = get_struct_fields_from_expr(context, node->access.src);
      }
    } else {
      node->access.datatype = infer_expr_type(context, node);
      node->access.fields = get_struct_fields_from_expr(context, node->access.src);
    }
    break;
  }

  case AST_STRUCT_DEFINITION: {
    Symbol *sym = lookup_token(context->codegen, node->struct_expr.name, node->struct_expr.fxn, node->struct_expr.level);
    if (sym) {
      report_semantic_error(context, "Multiple definition of struct.");
      break;
    }
    Symbol *res_symbol = register_variable(context->codegen, node->struct_expr.name, TYPE_STRUCT, node->struct_expr.fxn, node->struct_expr.level, node->struct_expr.name_length);
    res_symbol->struct_fields = node->struct_expr.fields;
    node->struct_expr.sym = res_symbol;

    Struct *field = node->struct_expr.fields;
    while (field) {
      if (!field->field || field->field->Type != AST_VAR_DECL) {
        report_semantic_error(context, "Expected variable declarations as fields.");
      }
      field = field->next;
    }
    break;
  }

  case AST_FUNCTIONS: {
    for (int i = 0; i < node->functions.count; i++) {
      if (node->functions.methods[i]) {
        analyze_node(context, node->functions.methods[i]);
      }
    }
    break;
  }
  }

  context->current_node = prev_node;
}

static void analyze_block(SemanticContext *context, ASTNode *block_node) {
  if (!block_node || block_node->Type != AST_BLOCK)
    return;

  int initial_symbol_count = context->codegen->symbol_count;

  for (int i = 0; i < block_node->block.count; ++i) {
    analyze_node(context, block_node->block.statements[i]);
  }

  for (int i = initial_symbol_count; i < context->codegen->symbol_count; ++i) {
    if (context->codegen->symbols[i].t_type != FUNC && context->codegen->symbols[i].type != TYPE_STRUCT) {
      context->codegen->symbols[i].is_active = false;
    }
  }
}
