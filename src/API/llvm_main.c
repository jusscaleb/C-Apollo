#include "../../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include "llvm-c/Types.h"
#include <stdio.h>
#include <time.h>

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

void _apl_llvm_environment_setup(CodegenContext *context,
                                 LLVMComponents *components,
                                 ASTNode *block_node) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("apl_module", ctx);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

  components->ctx = ctx;
  components->module = module;
  components->builder = builder;

  _apl_load_runtime_libraries(components);
  if (block_node != NULL && block_node->Type == AST_PROGRAM) {
    _apl_gen_block_from_ast(components, context, block_node->program.function);
  } else {
    _apl_gen_block_from_ast(components, context, block_node);
  }

  _apl_save_and_shutdown(components);
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
    fprintf(stderr, "LLVM IR successfully written to file\n");
  }

  _apl_llvm_shutdown(components);
}

void _apl_llvm_shutdown(LLVMComponents *components) {
  LLVMDisposeModule(components->module);
  LLVMDisposeBuilder(components->builder);
  LLVMContextDispose(components->ctx);
}

// AST Dictionary
void _apl_gen_block_from_ast(LLVMComponents *components,
                             CodegenContext *context, ASTNode *block_node) {
  if (block_node == NULL || block_node->Type != AST_BLOCK)
    return;

  for (int i = 0; i < block_node->block.count; i++) {
    ASTNode *stmt = block_node->block.statements[i];
    if (stmt == NULL)
      continue;

    switch (stmt->Type) {
    case AST_VAR_DECL:
      _apl_create_local_variable(components, context, stmt);
      break;
    case AST_VAR_ASS:
      _apl_reassign_variable(components, context, stmt);
      break;
    case AST_IF:
      _apl_gen_if_block(components, context, stmt);
      break;
    case AST_WHILE:
      _apl_gen_while_loop(components, context, stmt);
      break;
    case AST_FOR:
      _apl_gen_for_loop(components, context, stmt);
      break;
    case AST_CALL_FXN:
      _apl_gen_fxn_call_from_ast(components, context, stmt);
      break;
    case AST_RET_NODE:
      _apl_gen_function_end(components, context, stmt);

      break;
    case AST_FUNCTION: {
      _apl_gen_function_start(components, context, stmt);
      _apl_gen_block_from_ast(components, context, stmt->function.body);
      _apl_gen_function_end(components, context, stmt);

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


