#include "../../headers/semantic.h"
#include <stdio.h>

static void get_node_location(ASTNode *node, int *out_line, int *out_col) {
  if (!node) { *out_line = 0; *out_col = 0; return; }
  switch (node->Type) {
    case AST_VAR_DECL:
      *out_line = node->var_decl.fxn.line;
      *out_col = 0;
      break;
    case AST_VAR_REF:
      *out_line = node->var_ref.fxn.line;
      *out_col = 0;
      break;
    case AST_VAR_ASS:
      *out_line = node->var_assign.fxn.line;
      *out_col = 0;
      break;
    case AST_FUNCTION:
      *out_line = node->function.fxn.line;
      *out_col = 0;
      break;
    case AST_CALL_FXN:
      *out_line = node->call_fxn.fxn.line;
      *out_col = 0;
      break;
    case AST_LITERAL_EXPR:
      *out_line = node->literal_expr.token.line;
      *out_col = 0;
      break;
    case AST_BINARY_EXPR:
      if (node->binary_expr.left) get_node_location(node->binary_expr.left, out_line, out_col);
      break;
    case AST_STRUCT_DEFINITION:
      if (node->struct_expr.fxn) *out_line = node->struct_expr.fxn->line;
      break;
    case AST_RET_NODE:
      *out_line = node->ret_node.fxn.line;
      break;
    default:
      *out_line = 0;
      *out_col = 0;
      break;
  }
}

// Helper to push semantic errors
void report_semantic_error(SemanticContext *context, const char *msg) {
  if (!context) return;
  Error *e = malloc(sizeof(Error));
  e->token = (Token){0};
  e->type = SEMANTICERROR;
  e->message = strdup(msg);
  e->got = NULL;
  
  int line = 0, column = 0;
  if (context->current_node) {
    get_node_location(context->current_node, &line, &column);
  }
  e->line = line;
  e->column = column;
  errorStack_push(context->errors, e);
}

DataType infer_expr_type(SemanticContext *context, ASTNode *expr) {

  if (!expr)
    return TYPE_NULL;

  if (expr->Type == AST_LITERAL_EXPR) {

    if (expr->literal_expr.token.type == TOKEN_FLOAT)
      return TYPE_FLOAT;
    if (expr->literal_expr.token.type == TOKEN_BOOL)
      return TYPE_BOOL;
    if (expr->literal_expr.token.type == TOKEN_NULL)
      return TYPE_NULL;
    if (expr->literal_expr.token.type == TOKEN_STRING)
      return TYPE_STRING;
    if (expr->literal_expr.token.type == TOKEN_INT)
      return TYPE_INT;
    if (expr->literal_expr.token.type == TOKEN_CHAR)
      return TYPE_CHAR;
    return TYPE_NULL;
  }

  if (expr->Type == AST_BINARY_EXPR) {
    switch (expr->binary_expr.operator_type) {
    case TOKEN_GT:
    case TOKEN_ST:
    case TOKEN_SE:
    case TOKEN_GE:
    case TOKEN_EQT:
    case TOKEN_AND:
    case TOKEN_OR:
    case TOKEN_NEQ:
      return TYPE_BOOL;
    default: {
      DataType left_type = infer_expr_type(context, expr->binary_expr.left);
      DataType right_type = infer_expr_type(context, expr->binary_expr.right);
      
      if (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT)
        return TYPE_FLOAT;
      if (left_type == TYPE_CHAR || right_type == TYPE_CHAR)
        return TYPE_CHAR;

      return TYPE_INT;
    }
    }
  }
  if (expr->Type == AST_URINARY_EXPR) {
    switch (expr->urinary_expr.operator_type) {
    case TOKEN_REF:
    case TOKEN_MUL:
    case TOKEN_PTR:
    case TOKEN_SUB:
      return infer_expr_type(context, expr->urinary_expr.value);
    default:
      report_semantic_error(context, "Unrecognized operator type.");
      return TYPE_NULL;
    }
  }



  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[256];
    int len_v = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(var_name, expr->var_ref.name, len_v);
    var_name[len_v] = '\0';
    Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn,
                               expr->var_ref.level);
    if (sym) {
      return sym->type;
    } else {
      report_semantic_error(context,
                            "Unrecognized variable referenced in expression.");
    }
  }

  if (expr->Type == AST_CALL_FXN) {
    const int NAME_LENGTH = expr->call_fxn.name_length;
    char fn_name[256];
    int len_f = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(fn_name, expr->call_fxn.name, len_f);
    fn_name[len_f] = '\0';
    Symbol *sym = lookup_token(context->codegen, fn_name, &expr->call_fxn.fxn,
                               expr->call_fxn.level);
    if (sym) {
      return sym->type;
    }
  }

  if(expr->Type == AST_ARRAY_LITERAL){
    DataType elem_dt = expr->array_literal.element_type;
    if (elem_dt == TYPE_NULL && expr->array_literal.count > 0 && expr->array_literal.elements[0]) {
      elem_dt = infer_expr_type(context, expr->array_literal.elements[0]);
      expr->array_literal.element_type = elem_dt;
    }
    switch (elem_dt) {
      case TYPE_INT: return TYPE_INT_ARRAY;
      case TYPE_CHAR: return TYPE_CHAR_ARRAY;
      case TYPE_FLOAT: return TYPE_FLOAT_ARRAY;
      case TYPE_BOOL: return TYPE_BOOL_ARRAY;
      case TYPE_STRING: return TYPE_STR_ARRAY;
      case TYPE_BOOL_ARRAY:
      case TYPE_INT_ARRAY:
      case TYPE_FLOAT_ARRAY:
      case TYPE_STR_ARRAY:
      case TYPE_CHAR_ARRAY:
      case TYPE_STRUCT:
        return elem_dt;
      default: break;
    }
  }

  if (expr->Type == AST_INDEX_EXPR) {
    if (expr->index_expr.type != TYPE_NULL) {
      return expr->index_expr.type;
    }
    DataType target_type = infer_expr_type(context, expr->index_expr.target);
    switch (target_type) {
      case TYPE_INT_ARRAY:   return TYPE_INT;
      case TYPE_FLOAT_ARRAY: return TYPE_FLOAT;
      case TYPE_BOOL_ARRAY:  return TYPE_BOOL;
      case TYPE_CHAR_ARRAY:  return TYPE_CHAR;
      case TYPE_STR_ARRAY:   return TYPE_STRING;
      default:               return TYPE_NULL;
    }
  }

  if (expr->Type == AST_ACCESS) {
    if (expr->access.datatype != TYPE_NULL) {
      return expr->access.datatype;
    }
    if (expr->access.target && expr->access.target->Type == AST_CALL_FXN) {
      return expr->access.target->call_fxn.return_type;
    }
    Struct *fields = get_struct_fields_from_expr(context, expr->access.src);
    expr->access.fields = fields;
    if (!fields) {
      report_semantic_error(context, "Cannot access field on non-struct type.");
      return TYPE_NULL;
    }
    if (expr->access.target && expr->access.target->Type == AST_VAR_REF) {
      const char *field_name = expr->access.target->var_ref.name;
      int field_len = expr->access.target->var_ref.name_length;
      Struct *f = fields;
      while (f) {
        if (f->field && f->field->Type == AST_VAR_DECL) {
          if (f->field->var_decl.name_length == field_len &&
              memcmp(f->field->var_decl.name, field_name, field_len) == 0) {
            expr->access.datatype = f->field->var_decl.value_type;
            if (f->field->var_decl.value_type == TYPE_STRUCT) {
              expr->access.struct_type_name = f->field->var_decl.struct_type_name;
            }
            return expr->access.datatype;
          }
        }
        f = f->next;
      }
      report_semantic_error(context, "Field not found in struct definition.");
      return TYPE_NULL;
    }
  }

  report_semantic_error(context, "Could not infer expression type.");
  return TYPE_NULL;
}

Struct *get_struct_fields_from_expr(SemanticContext *context, ASTNode *expr) {
  if (!expr) return NULL;
  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[256];
    int len_v2 = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(var_name, expr->var_ref.name, len_v2);
    var_name[len_v2] = '\0';
    Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn,
                               expr->var_ref.level);
    if (sym && (sym->type == TYPE_STRUCT || sym->struct_fields)) {
      return sym->struct_fields;
    }
  } else if (expr->Type == AST_INDEX_EXPR) {
    if (expr->index_expr.target && expr->index_expr.target->Type == AST_VAR_REF) {
      const int NAME_LENGTH = expr->index_expr.target->var_ref.name_length;
      char var_name[256];
      int len_v3 = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
      memcpy(var_name, expr->index_expr.target->var_ref.name, len_v3);
      var_name[len_v3] = '\0';
      Symbol *sym = lookup_token(context->codegen, var_name, &expr->index_expr.target->var_ref.fxn,
                                 expr->index_expr.target->var_ref.level);
      if (sym && sym->struct_fields) {
        return sym->struct_fields;
      }
    }
  } else if (expr->Type == AST_ACCESS) {
    Struct *parent_fields = get_struct_fields_from_expr(context, expr->access.src);
    if (parent_fields && expr->access.target && expr->access.target->Type == AST_VAR_REF) {
      const char *field_name = expr->access.target->var_ref.name;
      int field_len = expr->access.target->var_ref.name_length;
      Struct *f = parent_fields;
      while (f) {
        if (f->field && f->field->Type == AST_VAR_DECL) {
          if (f->field->var_decl.name_length == field_len &&
              memcmp(f->field->var_decl.name, field_name, field_len) == 0) {
            if (f->field->var_decl.value_type == TYPE_STRUCT && f->field->var_decl.struct_type_name) {
              Symbol *s_sym = lookup_token(context->codegen, f->field->var_decl.struct_type_name,
                                           &f->field->var_decl.fxn, f->field->var_decl.level);
              if (s_sym) return s_sym->struct_fields;
            }
            return NULL;
          }
        }
        f = f->next;
      }
    }
  }
  return NULL;
}

int infer_expr_pointer_level(SemanticContext *context, ASTNode *expr) {
  if (!expr)
    return 0;

  if (expr->Type == AST_LITERAL_EXPR || expr->Type == AST_BINARY_EXPR) {
    return 0;
  }

  if (expr->Type == AST_URINARY_EXPR) {
    switch (expr->urinary_expr.operator_type) {
    case TOKEN_REF:
      return infer_expr_pointer_level(context, expr->urinary_expr.value) + 1;
    case TOKEN_MUL: {
      int inner_level = infer_expr_pointer_level(context, expr->urinary_expr.value);
      if (inner_level <= 0) {
        report_semantic_error(context, "Cannot dereference non-pointer type.");
        return 0;
      }
      return inner_level - 1;
    }
    case TOKEN_SUB:
      return infer_expr_pointer_level(context, expr->urinary_expr.value);
    default:
      return 0;
    }
  }

  if (expr->Type == AST_VAR_REF) {
    const int NAME_LENGTH = expr->var_ref.name_length;
    char var_name[256];
    int len_v4 = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(var_name, expr->var_ref.name, len_v4);
    var_name[len_v4] = '\0';
    Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn,
                               expr->var_ref.level);
    if (sym) {
      return sym->pointer_level;
    }
  }

  if (expr->Type == AST_CALL_FXN) {
    const int NAME_LENGTH = expr->call_fxn.name_length;
    char fn_name[256];
    int len_f2 = NAME_LENGTH < 255 ? NAME_LENGTH : 255;
    memcpy(fn_name, expr->call_fxn.name, len_f2);
    fn_name[len_f2] = '\0';
    Symbol *sym = lookup_token(context->codegen, fn_name, &expr->call_fxn.fxn,
                               expr->call_fxn.level);
    if (sym) {
      return sym->pointer_level;
    }
  }

  if (expr->Type == AST_INDEX_EXPR) {
    return infer_expr_pointer_level(context, expr->index_expr.target);
  }

  if (expr->Type == AST_ACCESS) {
    Struct *fields = get_struct_fields_from_expr(context, expr->access.src);
    if (fields && expr->access.target && expr->access.target->Type == AST_VAR_REF) {
      const char *field_name = expr->access.target->var_ref.name;
      int field_len = expr->access.target->var_ref.name_length;
      Struct *f = fields;
      while (f) {
        if (f->field && f->field->Type == AST_VAR_DECL) {
          if (f->field->var_decl.name_length == field_len &&
              memcmp(f->field->var_decl.name, field_name, field_len) == 0) {
            return f->field->var_decl.pointer_level;
          }
        }
        f = f->next;
      }
    }
    return 0;
  }

  return 0;
}


MemoryBucket infer_bucket_type(SemanticContext *context, ASTNode *expr){
  if(!expr)
    return 0;
  switch(expr->Type){
    case AST_BINARY_EXPR: {
      MemoryBucket right_b = infer_bucket_type(context, expr->binary_expr.right);
      MemoryBucket left_b = infer_bucket_type(context, expr->binary_expr.left);

      if (right_b == BUCKET_ONE) right_b = BUCKET_PLUS_ONE;
      if (left_b == BUCKET_ONE) left_b = BUCKET_PLUS_ONE;

      if (left_b == BUCKET_THREE || right_b == BUCKET_THREE) return BUCKET_THREE;
      if (left_b == BUCKET_TWO || right_b == BUCKET_TWO) return BUCKET_TWO;
      return BUCKET_PLUS_ONE;
    }
    case AST_INDEX_EXPR:
      return infer_bucket_type(context, expr->index_expr.target);
    case AST_ACCESS:
      return infer_bucket_type(context, expr->access.src);
    case AST_URINARY_EXPR:
      return infer_bucket_type(context, expr->urinary_expr.value);
    case AST_CALL_FXN:
      if (expr->call_fxn.resolved_symbol) return expr->call_fxn.resolved_symbol->bucket;
      return BUCKET_TWO;
    case AST_VAR_REF:
      if (expr->var_ref.resolved_symbol) return expr->var_ref.resolved_symbol->bucket;
      return BUCKET_PLUS_ONE;
    default: return BUCKET_PLUS_ONE;
  }
  return BUCKET_PLUS_ONE;
}

bool is_types_compatible(DataType expected, DataType inferred, int expected_ptr_level, int inferred_ptr_level, bool is_literal) {
  bool is_ptr_compatible = (inferred_ptr_level == expected_ptr_level) ||
                           (is_literal && expected_ptr_level > 0);

  if (!is_ptr_compatible) return false;

  if (expected == inferred) return true;
  if (expected == TYPE_INT && inferred == TYPE_CHAR) return true;
  if (expected == TYPE_CHAR && inferred == TYPE_INT) return true;

  // Array type compatibility
  if ((expected == TYPE_INT_ARRAY || expected == TYPE_INT) &&
      (inferred == TYPE_INT_ARRAY || inferred == TYPE_INT)) return true;
  if ((expected == TYPE_FLOAT_ARRAY || expected == TYPE_FLOAT) &&
      (inferred == TYPE_FLOAT_ARRAY || inferred == TYPE_FLOAT)) return true;
  if ((expected == TYPE_BOOL_ARRAY || expected == TYPE_BOOL) &&
      (inferred == TYPE_BOOL_ARRAY || inferred == TYPE_BOOL)) return true;
  if ((expected == TYPE_STR_ARRAY || expected == TYPE_STRING) &&
      (inferred == TYPE_STR_ARRAY || inferred == TYPE_STRING)) return true;
  if ((expected == TYPE_CHAR_ARRAY || expected == TYPE_CHAR) &&
      (inferred == TYPE_CHAR_ARRAY || inferred == TYPE_CHAR)) return true;
  if (expected == TYPE_STRUCT && (inferred == TYPE_STRUCT || inferred == TYPE_INT_ARRAY || inferred == TYPE_NULL)) return true;

  return false;
}
