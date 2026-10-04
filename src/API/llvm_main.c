#include "../../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include "llvm-c/Error.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Transforms/PassBuilder.h"
#include "llvm-c/Types.h"

static FILE *trace_file = NULL;

static void trace(const char *message) {
  if (!trace_file) {
    trace_file = fopen("llvm.log", "w");
  }
  if (trace_file) {
    fprintf(trace_file, "%s\n", message);
    fflush(trace_file);
  }
}
void _apl_load_runtime_libraries(LLVMComponents *components) {
  const char *libs[] = RUNTIME_LIBS;
  const size_t libs_count = sizeof(libs) / sizeof(libs[0]);

  for (size_t i = 0; i < libs_count; i++) {
    _apl_import_runtime(components->module, libs[i]);
  }
}

void _apl_import_runtime(LLVMModuleRef dest_module, const char *filename) {
  LLVMModuleRef src_module;
  LLVMMemoryBufferRef buffer = NULL;
  char *msg = NULL;

  int success =
      LLVMCreateMemoryBufferWithContentsOfFile(filename, &buffer, &msg);

  if (success != 0) {
    fprintf(stderr, "Error: Could not create memory buffer from file\n");
    LLVMDisposeMessage(msg);
    return;
  }

  int parse_bc = LLVMParseBitcodeInContext2(
      dest_module ? LLVMGetModuleContext(dest_module) : LLVMGetGlobalContext(),
      buffer, &src_module);

  if (parse_bc != 0) {
    fprintf(stderr, "Error parsing bitcode: %s\n", msg);
    LLVMDisposeMessage(msg);
    return;
  }

  int merge_src = LLVMLinkModules2(dest_module, src_module);

  if (merge_src != 0) {
    fprintf(stderr, "Error linking modules!\n");
    if (msg) {
      LLVMDisposeMessage(msg);
    }
    return;
  }

  LLVMDisposeMemoryBuffer(buffer);
  if (msg) {
    LLVMDisposeMessage(msg);
  }
}

void _apl_llvm_environment_setup(LLVMComponents *components,
                                 ASTNode *block_node) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("apl_module", ctx);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

  components->ctx = ctx;
  components->module = module;
  components->builder = builder;

  _apl_load_runtime_libraries(components);
  if (block_node != NULL && block_node->Type == AST_PROGRAM) {
    _apl_gen_block_from_ast(components, block_node->program.function);
  } else {
    _apl_gen_block_from_ast(components, block_node);
  }

  _apl_optimize_module(components);
}
static LLVMValueRef _apl_get_runtime_function(LLVMComponents *components,
                                              const char *name,
                                              LLVMTypeRef fn_type) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, name);
  if (fn != NULL) {
    return fn;
  }
  return LLVMAddFunction(components->module, name, fn_type);
}

void _apl_save_and_shutdown(LLVMComponents *components) {

  char *ir = LLVMPrintModuleToString(components->module);
  trace("LLVM Code:\n");
  trace(ir);
  char *verify_msg = NULL;
  if (LLVMVerifyModule(components->module, LLVMPrintMessageAction,
                       &verify_msg) != 0) {
    fprintf(stderr, "LLVM module verification failed:\n%s\n",
            verify_msg ? verify_msg : "(no verifier message)");
    if (verify_msg) {
      LLVMDisposeMessage(verify_msg);
    }
    _apl_llvm_shutdown(components);
    exit(EXIT_FAILURE);
  }
  if (verify_msg) {
    LLVMDisposeMessage(verify_msg);
  }

  const int save_err =
      LLVMWriteBitcodeToFile(components->module, "temp\\output.bc");

  if (save_err != 0) {
    fprintf(stderr, "Error: Could not write LLVM IR to file (code %d)\n",
            save_err);
  } else {
    _DEBUG("LLVM IR successfully written to file.")
  }

  _apl_llvm_shutdown(components);
}

void _apl_llvm_shutdown(LLVMComponents *components) {
  LLVMDisposeModule(components->module);
  LLVMDisposeBuilder(components->builder);
  LLVMContextDispose(components->ctx);
}

void _apl_optimize_module(LLVMComponents *components) {
  if (!components || !components->module)
    return;

  char *verify_msg = NULL;
  if (LLVMVerifyModule(components->module, LLVMPrintMessageAction,
                       &verify_msg) != 0) {
    fprintf(stderr, "[LLVM Backend Error] Module verification failed before optimization:\n%s\n",
            verify_msg ? verify_msg : "(no verifier message)");
    if (verify_msg) {
      LLVMDisposeMessage(verify_msg);
    }
    _apl_save_and_shutdown(components);
    exit(EXIT_FAILURE);
  }
  if (verify_msg) {
    LLVMDisposeMessage(verify_msg);
  }

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmPrinter();
  LLVMInitializeNativeAsmParser();

  // Fetch host machine triple, CPU model, and feature flags
  char *target_triple = LLVMGetDefaultTargetTriple();
  char *cpu = LLVMGetHostCPUName();
  char *features = LLVMGetHostCPUFeatures();

  LLVMTargetRef target;
  char *err_msg = NULL;

  if (LLVMGetTargetFromTriple(target_triple, &target, &err_msg)) {
    if (err_msg)
      LLVMDisposeMessage(err_msg);
    LLVMDisposeMessage(target_triple);
    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);
    return;
  }

  LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
      target, target_triple, cpu, features, LLVMCodeGenLevelAggressive,
      LLVMRelocDefault, LLVMCodeModelDefault);

  LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();

  LLVMErrorRef err =
      LLVMRunPasses(components->module, "default<O3>", target_machine, options);

  if (err) {
    LLVMConsumeError(err);
  }

  LLVMDisposePassBuilderOptions(options);

  // Emit native object file directly from the optimized module.
  char *obj_err = NULL;
  if (LLVMTargetMachineEmitToFile(target_machine, components->module,
                                   "temp\\output.o", LLVMObjectFile,
                                   &obj_err)) {
    fprintf(stderr, "[LLVM Backend Error] Failed to emit object file: %s\n",
            obj_err ? obj_err : "(unknown)");
    if (obj_err) LLVMDisposeMessage(obj_err);
  } else {
    _DEBUG("Native object file emitted to temp/output.o")
  }

  LLVMDisposeTargetMachine(target_machine);
  LLVMDisposeMessage(target_triple);
  LLVMDisposeMessage(cpu);
  LLVMDisposeMessage(features);

  _apl_save_and_shutdown(components);
}

void _apl_gen_block_from_ast(LLVMComponents *components, ASTNode *block_node) {
  if (block_node == NULL || block_node->Type != AST_BLOCK)
    return;

  for (int i = 0; i < block_node->block.count; i++) {
    ASTNode *stmt = block_node->block.statements[i];
    if (stmt == NULL)
      continue;
    fflush(stdout);

    switch (stmt->Type) {
    case AST_VAR_DECL:
      _apl_create_local_variable(components, stmt);
      break;
    case AST_VAR_ASS:
      _apl_reassign_variable(components, stmt);
      break;
    case AST_IF:
      _apl_gen_if_block(components, stmt);
      break;
    case AST_WHILE:
      _apl_gen_while_loop(components, stmt);
      break;
    case AST_FOR:
      _apl_gen_for_loop(components, stmt);
      break;
    case AST_CALL_FXN:
      _apl_gen_fxn_call_from_ast(components, stmt);
      break;
    case AST_ACCESS:
      _apl_gen_fxn_call_from_ast(components, stmt);
      break;
    case AST_RET_NODE:
      _apl_gen_return(components, stmt);
      break;
    case AST_STRUCT_DEFINITION:
      _apl_gen_struct_definition(components, stmt);
      break;
    case AST_FUNCTIONS: {
      for (int m_idx = 0; m_idx < stmt->functions.count; m_idx++) {
        if (stmt->functions.methods[m_idx]) {
          ASTNode *m_node = stmt->functions.methods[m_idx];
          LLVMValueRef parent_fxn = components->current_fxn;
          LLVMBasicBlockRef parent_block =
              (components->builder) ? LLVMGetInsertBlock(components->builder)
                                    : NULL;
          LLVMTypeRef parent_frame_type = components->current_frame_type;
          LLVMValueRef parent_frame_alloc = components->current_frame_alloc;
          LLVMValueRef parent_parent_frame = components->current_parent_frame;
          LLVMValueRef parent_arena_ptr = components->current_arena_ptr;
          LLVMValueRef parent_target_return_arena = components->target_return_arena;
          Fxn *parent_fxn_ast = components->current_fxn_ast;

          _apl_gen_function_start(components, m_node);
          _apl_gen_block_from_ast(components, m_node->function.body);

          _apl_gen_function_end(components, m_node);

          components->current_fxn = parent_fxn;
          components->current_frame_type = parent_frame_type;
          components->current_frame_alloc = parent_frame_alloc;
          components->current_parent_frame = parent_parent_frame;
          components->current_arena_ptr = parent_arena_ptr;
          components->target_return_arena = parent_target_return_arena;
          components->current_fxn_ast = parent_fxn_ast;
          if (parent_block && components->builder) {
            LLVMPositionBuilderAtEnd(components->builder, parent_block);
          }
        }
      }
      break;
    }
    case AST_FUNCTION: {
      LLVMValueRef parent_fxn = components->current_fxn;
      LLVMBasicBlockRef parent_block =
          (components->builder) ? LLVMGetInsertBlock(components->builder)
                                : NULL;
      LLVMTypeRef parent_frame_type = components->current_frame_type;
      LLVMValueRef parent_frame_alloc = components->current_frame_alloc;
      LLVMValueRef parent_parent_frame = components->current_parent_frame;
      LLVMValueRef parent_arena_ptr = components->current_arena_ptr;
      LLVMValueRef parent_target_return_arena = components->target_return_arena;
      Fxn *parent_fxn_ast = components->current_fxn_ast;

      _apl_gen_function_start(components, stmt);
      _apl_gen_block_from_ast(components, stmt->function.body);

      _apl_gen_function_end(components, stmt);

      components->current_fxn = parent_fxn;
      components->current_frame_type = parent_frame_type;
      components->current_frame_alloc = parent_frame_alloc;
      components->current_parent_frame = parent_parent_frame;
      components->current_arena_ptr = parent_arena_ptr;
      components->target_return_arena = parent_target_return_arena;
      components->current_fxn_ast = parent_fxn_ast;
      if (parent_block != NULL) {
        LLVMPositionBuilderAtEnd(components->builder, parent_block);
      }

      break;
    }

    default:
      fprintf(stderr, "Unsupported statement type in block codegen. %d\n",
              stmt->Type);
      _apl_save_and_shutdown(components);
      exit(EXIT_FAILURE);
    }
  }
}
