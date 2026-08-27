/*===================================================================
                         llvm_memory.c

                    (c)2026 SCXRPIUS.dev

      LLVM Memory Management scaffolding for 3+1 Bucket Memory
      Architecture (CPU Stack, Static Segment, Scoped Region Arena,
      and Heap ARC).
====================================================================*/

#include "../../headers/llvm_backend.h"
#include <stdio.h>
#include <stdbool.h>


LLVMValueRef _apl_emit_arena_create(LLVMComponents *components, uint64_t capacity) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_create");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMInt64TypeInContext(components->ctx) };
    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arena_create", fn_type);
  }

  LLVMValueRef args[] = { LLVMConstInt(LLVMInt64TypeInContext(components->ctx), capacity, false) };
  return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "arena_create_call");
}

LLVMValueRef _apl_emit_arena_reset(LLVMComponents *components, LLVMValueRef arena_ptr) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_reset");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0) };
    LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arena_reset", fn_type);
  }

  LLVMValueRef args[] = { arena_ptr };
  return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "");
}

LLVMValueRef _apl_emit_arena_destroy(LLVMComponents *components, LLVMValueRef arena_ptr) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_destroy");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0) };
    LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arena_destroy", fn_type);
  }

  LLVMValueRef args[] = { arena_ptr };
  return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "");
}

LLVMValueRef _apl_emit_heap_alloc_arc(LLVMComponents *components, LLVMValueRef size_val) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_heap_alloc_arc");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMInt64TypeInContext(components->ctx) };
    LLVMTypeRef fn_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_heap_alloc_arc", fn_type);
  }

  LLVMValueRef args[] = { size_val };
  return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "heap_arc_alloc");
}

void _apl_emit_arc_retain(LLVMComponents *components, LLVMValueRef heap_ptr) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arc_retain");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0) };
    LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arc_retain", fn_type);
  }
  LLVMValueRef args[] = { heap_ptr };
  LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "");
}

void _apl_emit_arc_release(LLVMComponents *components, LLVMValueRef heap_ptr) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arc_release");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0) };
    LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arc_release", fn_type);
  }
  LLVMValueRef args[] = { heap_ptr };
  LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "");
}

LLVMValueRef _apl_allocate_variable_by_bucket(LLVMComponents *components, Symbol *sym, LLVMTypeRef var_type) {
  if (!sym) return NULL;

  switch (sym->bucket) {
    case BUCKET_ONE: {
      LLVMValueRef global_var = LLVMAddGlobal(components->module, var_type, sym->name);
      LLVMSetGlobalConstant(global_var, true);
      LLVMSetInitializer(global_var, LLVMConstNull(var_type));
      return global_var;
    }
    case BUCKET_TWO: {
      LLVMValueRef size_val = LLVMSizeOf(var_type);
      LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_grow_and_alloc");
      if (!fn) {
        LLVMTypeRef param_types[] = {
          LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
          LLVMInt64TypeInContext(components->ctx)
        };
        LLVMTypeRef fn_type = LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0), param_types, 2, false);
        fn = LLVMAddFunction(components->module, "apl_arena_grow_and_alloc", fn_type);
      }
      LLVMValueRef arena_to_use = (components->target_return_arena != NULL) ? components->target_return_arena : components->current_arena_ptr;
      LLVMValueRef args[] = { arena_to_use, size_val };
      return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 2, "");
    }

    case BUCKET_THREE: {
      LLVMValueRef size_val = LLVMSizeOf(var_type);
        LLVMValueRef raw_ptr = _apl_emit_heap_alloc_arc(components, size_val);
        return LLVMBuildBitCast(components->builder, raw_ptr, LLVMPointerType(var_type, 0), sym->name);
    }

    case BUCKET_PLUS_ONE:
    default: {
      return LLVMBuildAlloca(components->builder, var_type, sym->name);
    }
  }
}

LLVMValueRef _apl_emit_arena_get_mark(LLVMComponents *components, LLVMValueRef arena_ptr) {
  if (!arena_ptr) return NULL;
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_get_mark");
  if (!fn) {
    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0) };
    LLVMTypeRef fn_type = LLVMFunctionType(LLVMInt64TypeInContext(components->ctx), param_types, 1, false);
    fn = LLVMAddFunction(components->module, "apl_arena_get_mark", fn_type);
  }
  LLVMValueRef args[] = { arena_ptr };
  return LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 1, "arena_mark");
}

void _apl_emit_arena_set_mark(LLVMComponents *components, LLVMValueRef arena_ptr, LLVMValueRef mark_val) {
  if (!arena_ptr || !mark_val) return;
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, "apl_arena_set_mark");
  if (!fn) {
    LLVMTypeRef param_types[] = {
      LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
      LLVMInt64TypeInContext(components->ctx)
    };
    LLVMTypeRef fn_type = LLVMFunctionType(VOID(components->ctx), param_types, 2, false);
    fn = LLVMAddFunction(components->module, "apl_arena_set_mark", fn_type);
  }
  LLVMValueRef args[] = { arena_ptr, mark_val };
  LLVMBuildCall2(components->builder, LLVMGlobalGetValueType(fn), fn, args, 2, "");
}
