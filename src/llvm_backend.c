#include "../headers/llvm_backend.h"
#include "llvm-c/Core.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Types.h"
#include <string.h>

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

  // 1. Create a memory buffer from the file
  int success =
      LLVMCreateMemoryBufferWithContentsOfFile(filename, &buffer, &msg);

  if (success != 0) {
    fprintf(stderr, "Error: Could not create memory buffer from file\n");
    //LLVMDisposeMemoryBuffer(buffer);
    LLVMDisposeMessage(msg);
    return;
  }

  // 2. Parse the bitcode into the same context as the destination module.
  int parse_bc = LLVMParseBitcodeInContext2(dest_module ? LLVMGetModuleContext(dest_module) : LLVMGetGlobalContext(),
                                            buffer, &src_module);

  if (parse_bc != 0) {
    fprintf(stderr, "Error parsing bitcode: %s\n", msg);
    //LLVMDisposeMemoryBuffer(buffer);
    LLVMDisposeMessage(msg);
    return;
  }

  // 3. Link: Merge src into dest
  int merge_src = LLVMLinkModules2(dest_module, src_module);

  if (merge_src != 0) {
    fprintf(stderr, "Error linking modules!\n");
    //LLVMDisposeMemoryBuffer(buffer);
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

LLVMValueRef _apl_get_or_declare_printf(LLVMComponents *components) {
  LLVMValueRef printf_fn = LLVMGetNamedFunction(components->module, "printf");
  if (printf_fn != NULL) {
    return printf_fn;
  }

  LLVMTypeRef i8_ptr = LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
  LLVMTypeRef params[] = {i8_ptr};
  LLVMTypeRef printf_ty = LLVMFunctionType(
      LLVMInt32TypeInContext(components->ctx), params, 1, true);
  return LLVMAddFunction(components->module, "printf", printf_ty);
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
  if (LLVMVerifyModule(components->module, LLVMPrintMessageAction, &verify_msg) != 0) {
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
    case AST_PRINTLN:
      _apl_gen_println_ir(components, context, stmt);
      break;
    case AST_VAR_DECL:
      _apl_create_local_variable(components, context, stmt);
      break;
    case AST_VAR_ASS:
      // gen_var_assign_from_ast(context, stmt);
      break;
    case AST_IF:
      // gen_if_from_ast(context, stmt);
      break;

    case AST_WHILE:
      // gen_while_from_ast(context, stmt);
      break;
    case AST_FOR:
      // gen_for_from_ast(context, stmt);
      break;
    case AST_CALL_FXN:
      // gen_fxn_call_from_ast(context, stmt);
      break;
    case AST_RET_NODE:
      // gen_return_from_ast(context, stmt);
      break;
    case AST_FUNCTION: {
      /*if (stmt->function.fxn.parent_fxn != NULL &&
      strcmp(stmt->function.fxn.parent_fxn->name, "global") != 0) {
        // It's a nested function, defer it to avoid nested LLVM definitions!
        if (context->deferred_count >= context->deferred_capacity) {
          context->deferred_capacity = context->deferred_capacity == 0 ? 8 :
      context->deferred_capacity * 2; context->deferred_functions =
      realloc(context->deferred_functions, context->deferred_capacity *
      sizeof(ASTNode*));
        }
        context->deferred_functions[context->deferred_count++] = stmt;
      } else {
        // It's a global function, generate normally
        gen_function_start(context, stmt);
        gen_block_from_ast(context, stmt->function.body);
        gen_function_end(context,
      strcmp(stmt->function.resolved_symbol->llvm_name, "run") == 0,
      stmt->function.fxn.return_type);
      }*/

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

void _apl_gen_function_start(LLVMComponents *components,
                             CodegenContext *context, ASTNode *block_node) {
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMTypeRef main_fxn_type =
    LLVMFunctionType(LLVMInt32TypeInContext(components->ctx), NULL, 0, false);
    LLVMValueRef main_fxn = LLVMAddFunction(components->module, "main", main_fxn_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(components->ctx, main_fxn, "entry");
    LLVMPositionBuilderAtEnd(components->builder, entry);
    return;
  }

  else{
    LLVMTypeRef fxn_type =LLVMFunctionType(_enquire_fxn_return_type(components, &block_node->function.fxn), NULL, 0, false);
    LLVMValueRef fxn = LLVMAddFunction(components->module, block_node->function.name, fxn_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(components->ctx, fxn, "entry");
    LLVMPositionBuilderAtEnd(components->builder, entry);
    return;
  }
}

void _apl_gen_function_end(LLVMComponents *components, CodegenContext *context, ASTNode *block_node){
  if(memcmp(block_node->function.name, "run", 3) == 0){
    LLVMBuildRet(components->builder, LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false));
  }
}


LLVMTypeRef _enquire_fxn_return_type(LLVMComponents *components, Fxn *fxn){
    switch(fxn->return_type){
        case TYPE_INT:
            return LLVMInt32TypeInContext(components->ctx);
        case TYPE_FLOAT:
            return LLVMFloatTypeInContext(components->ctx);
        case TYPE_STRING:
            return LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
        default:
            return LLVMVoidTypeInContext(components->ctx);
    }
}


LLVMValueRef get_or_create_string_literal(LLVMComponents *components, const char *str, int length) {
    
    LLVMValueRef const_str = LLVMConstStringInContext(components->ctx, str, length, false);
    LLVMValueRef global_var = LLVMAddGlobal(components->module, LLVMTypeOf(const_str), ".str_lit");
    
    LLVMSetInitializer(global_var, const_str);
    LLVMSetGlobalConstant(global_var, true);
    LLVMSetLinkage(global_var, LLVMPrivateLinkage);
    LLVMSetUnnamedAddress(global_var, LLVMGlobalUnnamedAddr);

    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false),
        LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false)
    };
    return LLVMBuildInBoundsGEP2(components->builder, LLVMTypeOf(const_str), global_var, indices, 2, "str_ptr");
}


void _apl_gen_println_ir(LLVMComponents *components, CodegenContext *context, ASTNode *block_node){
    ASTNode *val = block_node->println.value;

    if(val == NULL)return;


    LLVMValueRef str = NULL;

    if(val->Type == AST_LITERAL_EXPR){
            Token token = val->literal_expr.token;

        if(token.type == TOKEN_STRING){
            str = get_or_create_string_literal(components, token.start + 1, token.length - 1);

        }else {
            str = get_or_create_string_literal(components, token.start, token.length);
        }
    }
    else{
    str = get_or_create_string_literal(components, "", 0 ); 

    }
    LLVMTypeRef i8_ptr = LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
    LLVMTypeRef print_string_ty =
        LLVMFunctionType(LLVMVoidTypeInContext(components->ctx), &i8_ptr, 1, false);
    LLVMTypeRef no_args_ty =
        LLVMFunctionType(LLVMVoidTypeInContext(components->ctx), NULL, 0, false);

    LLVMValueRef print_string_fn = _apl_get_runtime_function(components, "_apl_print_string", print_string_ty);
    LLVMValueRef newline_fn = _apl_get_runtime_function(components, "_apl_print_newline", no_args_ty);
    LLVMValueRef args[] = {str};
    LLVMBuildCall2(components->builder, print_string_ty, print_string_fn, args, 1, "");
    LLVMBuildCall2(components->builder, no_args_ty, newline_fn, NULL, 0, "");
}


void _apl_create_local_variable(LLVMComponents *components, CodegenContext *context, ASTNode *var_node){
    char var_name[var_node->var_decl.name_length + 1];
    memcpy(var_name, var_node->var_decl.name, var_node->var_decl.name_length);
    var_name[var_node->var_decl.name_length] = '\0';

    const DataType VAR_TYPE = var_node->var_decl.value_type;


    LLVMValueRef var_ptr;
    LLVMValueRef assign_val;
    LLVMValueRef var_val;

    switch(VAR_TYPE){

      case TYPE_INT:
        var_ptr = LLVMBuildAlloca(components->builder, LLVMInt32TypeInContext(components->ctx), var_name);
        assign_val = LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false);
        LLVMBuildStore(components->builder, assign_val, var_ptr);
        break;      

      default:
        break;
      

    }

        /*LLVMValueRef x_ptr = LLVMBuildAlloca(components->builder, LLVMInt32Type(), "x");

        LLVMValueRef const_10 = LLVMConstInt(LLVMInt32Type(), 10, 0);
        LLVMBuildStore(components->builder, const_10, x_ptr);

        LLVMValueRef x_val = LLVMBuildLoad2(components->builder, LLVMInt32Type(), x_ptr, "x_val");


        LLVMBuildRet(components->builder, x_val);*/
}

//===================================================HELPERS======================================================>
