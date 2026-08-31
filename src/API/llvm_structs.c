/*===================================================================
                         llvm_structs.c

                    (c)2026 SCXRPIUS.dev

      LLVM Code Generation for Struct Definitions and Field Access.
====================================================================*/

#include "../../headers/llvm_backend.h"
#include <string.h>

LLVMTypeRef _apl_get_field_llvm_type(LLVMComponents *components, ASTNode *field_decl) {
  if (!field_decl || field_decl->Type != AST_VAR_DECL) {
    return I32(components->ctx);
  }

  // 1. Pointer fields (int*, Node*, etc.)
  if (field_decl->var_decl.pointer_level > 0) {
    return LLVMPointerType(I8(components->ctx), 0);
  }

  // 2. Fixed-size array fields (int[5], etc.)
  if (field_decl->var_decl.array_count > 0) {
    DataType elem_dt = TYPE_INT;
    switch (field_decl->var_decl.value_type) {
      case TYPE_FLOAT_ARRAY: elem_dt = TYPE_FLOAT; break;
      case TYPE_CHAR_ARRAY:  elem_dt = TYPE_CHAR; break;
      case TYPE_BOOL_ARRAY:  elem_dt = TYPE_BOOL; break;
      case TYPE_STR_ARRAY:   elem_dt = TYPE_STRING; break;
      default:               elem_dt = TYPE_INT; break;
    }
    LLVMTypeRef elem_type = _apl_get_llvm_type(components, elem_dt);
    return LLVMArrayType(elem_type, field_decl->var_decl.array_count);
  }

  // 3. Primitive & Aggregate Data Types
  switch (field_decl->var_decl.value_type) {
    case TYPE_INT:
      return I32(components->ctx);

    case TYPE_FLOAT:
      return F32(components->ctx);

    case TYPE_BOOL:
      return I1(components->ctx);

    case TYPE_CHAR:
      return I8(components->ctx);

    case TYPE_STRING: {
      LLVMTypeRef str_members[] = {
        LLVMPointerType(I8(components->ctx), 0),
        I32(components->ctx)
      };
      return LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    }

    case TYPE_STRUCT: {
      const char *type_name = field_decl->var_decl.struct_type_name ? field_decl->var_decl.struct_type_name : field_decl->var_decl.name;
      int name_len = field_decl->var_decl.struct_type_name ? field_decl->var_decl.struct_type_name_len : field_decl->var_decl.name_length;
      char s_name[name_len + 1];
      memcpy(s_name, type_name, name_len);
      s_name[name_len] = '\0';
      LLVMTypeRef nested = LLVMGetTypeByName(components->module, s_name);
      if (nested) return nested;
      return LLVMPointerType(I8(components->ctx), 0);
    }

    default:
      return _apl_get_llvm_type(components, field_decl->var_decl.value_type);
  }
}

LLVMTypeRef _apl_gen_struct_definition(LLVMComponents *components, ASTNode *struct_node) {
  if (!struct_node || struct_node->Type != AST_STRUCT_DEFINITION) {
    return NULL;
  }

  const int NAME_LENGTH = struct_node->struct_expr.name_length;
  char name[NAME_LENGTH + 1];
  memcpy(name, struct_node->struct_expr.name, NAME_LENGTH);
  name[NAME_LENGTH] = '\0';

  // 1. Create or retrieve named LLVM struct
  LLVMTypeRef struct_type = LLVMGetTypeByName(components->module, name);
  if (!struct_type) {
    struct_type = LLVMStructCreateNamed(components->ctx, name);
  }

  // 2. Count fields
  uint32_t field_count = 0;
  Struct *f = struct_node->struct_expr.fields;
  while (f) {
    if (f->field) field_count++;
    f = f->next;
  }

  // 3. Collect LLVM types for all fields
  LLVMTypeRef field_types[field_count > 0 ? field_count : 1];
  uint32_t idx = 0;
  f = struct_node->struct_expr.fields;
  while (f) {
    if (f->field) {
      field_types[idx++] = _apl_get_field_llvm_type(components, f->field);
    }
    f = f->next;
  }

  // 4. Set struct body with natural memory alignment (isPacked = false) for maximum CPU efficiency
  LLVMStructSetBody(struct_type, field_types, field_count, false);

  // 5. Store type reference in symbol table
  if (struct_node->struct_expr.sym) {
    struct_node->struct_expr.sym->llvm_struct_type = struct_type;
  }

  return struct_type;
}

LLVMValueRef _apl_gen_struct_field_gep(LLVMComponents *components, LLVMValueRef struct_ptr,
                                       LLVMTypeRef struct_type, int field_index, const char *field_name) {
  return LLVMBuildStructGEP2(components->builder, struct_type, struct_ptr, field_index, field_name);
}

LLVMValueRef _apl_get_struct_access_ptr(LLVMComponents *components, ASTNode *access_node) {
  if (!access_node || access_node->Type != AST_ACCESS)
    return NULL;

  ASTNode *src = access_node->access.src;
  if (!src)
    return NULL;

  LLVMValueRef struct_ptr = NULL;
  LLVMTypeRef struct_type = NULL;
  Struct *fields = access_node->access.fields;

  if (src->Type == AST_VAR_REF) {
    Symbol *sym = src->var_ref.resolved_symbol;
    if (sym) {
      struct_ptr = sym->llvm_val_ref;
      struct_type = sym->llvm_struct_type;
      if (!fields) fields = sym->struct_fields;
      if (!struct_type && sym->struct_type_name) {
        struct_type = LLVMGetTypeByName(components->module, sym->struct_type_name);
      }
      if (!struct_type && sym->name) {
        struct_type = LLVMGetTypeByName(components->module, sym->name);
      }
      if (struct_ptr && (sym->pointer_level > 0 || sym->bucket == BUCKET_THREE)) {
        if (struct_type) {
          struct_ptr = LLVMBuildLoad2(components->builder, LLVMPointerType(struct_type, 0), struct_ptr, "deref_struct_ptr");
        } else {
          struct_ptr = LLVMBuildLoad2(components->builder, LLVMPointerType(I8(components->ctx), 0), struct_ptr, "deref_struct_ptr");
        }
      }
    }
    if (!struct_ptr) {
      struct_ptr = _apl_get_lvalue_address(components, src);
    }
  } else if (src->Type == AST_INDEX_EXPR) {
    struct_ptr = _apl_get_lvalue_address(components, src);
    if (src->index_expr.target && src->index_expr.target->Type == AST_VAR_REF) {
      Symbol *sym = src->index_expr.target->var_ref.resolved_symbol;
      if (sym) {
        struct_type = sym->llvm_struct_type;
        if (!fields) fields = sym->struct_fields;
        if (!struct_type && sym->name) {
          struct_type = LLVMGetTypeByName(components->module, sym->name);
        }
      }
    }
  } else if (src->Type == AST_ACCESS) {
    struct_ptr = _apl_get_struct_access_ptr(components, src);
    if (src->access.struct_type_name) {
      struct_type = LLVMGetTypeByName(components->module, src->access.struct_type_name);
    }
  }

  if (!struct_ptr)
    return NULL;

  if (!struct_type) {
    if (LLVMIsAAllocaInst(struct_ptr)) {
      struct_type = LLVMGetAllocatedType(struct_ptr);
    } else if (LLVMIsAGlobalVariable(struct_ptr)) {
      struct_type = LLVMGetInitializer(struct_ptr) ? LLVMTypeOf(LLVMGetInitializer(struct_ptr)) : NULL;
    }
  }

  if (!struct_type)
    return NULL;

  ASTNode *target = access_node->access.target;
  if (!target || target->Type != AST_VAR_REF)
    return NULL;

  const char *field_name = target->var_ref.name;
  int field_len = target->var_ref.name_length;

  int target_index = 0;
  int idx = 0;
  Struct *f = fields;
  while (f) {
    if (f->field && f->field->Type == AST_VAR_DECL) {
      if (f->field->var_decl.name_length == field_len &&
          memcmp(f->field->var_decl.name, field_name, field_len) == 0) {
        target_index = idx;
        break;
      }
      idx++;
    }
    f = f->next;
  }

  char f_name[field_len + 1];
  memcpy(f_name, field_name, field_len);
  f_name[field_len] = '\0';

  return LLVMBuildStructGEP2(components->builder, struct_type, struct_ptr,
                             target_index, f_name);
}

LLVMValueRef _apl_gen_struct_access_load(LLVMComponents *components,
                                         ASTNode *access_node) {
  if (!access_node || access_node->Type != AST_ACCESS)
    return NULL;

  LLVMValueRef field_ptr = _apl_get_struct_access_ptr(components, access_node);
  if (!field_ptr)
    return NULL;

  DataType dt = access_node->access.datatype;
  if (dt == TYPE_STRING) {
    LLVMTypeRef str_members[] = {LLVMPointerType(I8(components->ctx), 0),
                                 I32(components->ctx)};
    LLVMTypeRef string_struct_type =
        LLVMStructTypeInContext(components->ctx, str_members, 2, false);
    LLVMValueRef char_ptr_gep = LLVMBuildStructGEP2(
        components->builder, string_struct_type, field_ptr, 0, "str_gep");
    return LLVMBuildLoad2(components->builder,
                          LLVMPointerType(I8(components->ctx), 0),
                          char_ptr_gep, "str_val");
  }

  LLVMTypeRef elem_type = _apl_get_llvm_type(components, dt);
  if (!elem_type || LLVMGetTypeKind(elem_type) == LLVMVoidTypeKind) {
    elem_type = I32(components->ctx);
  }

  return LLVMBuildLoad2(components->builder, elem_type, field_ptr, "access_val");
}
