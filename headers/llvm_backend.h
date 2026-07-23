/*===================================================================
                         llvm_backend.h

                    (c)2026 SCXRPIUS.dev

          LLVM API backend scaffold for Apollo code generation.
====================================================================*/

#ifndef APOLLO_LLVM_BACKEND_H
#define APOLLO_LLVM_BACKEND_H

#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include "ast.h"
#include "defs.h"
#include "variables.h"
#include <llvm-c/Linker.h>
#include <llvm-c/BitReader.h>
#include "../headers/semantic.h"

#define RUNTIME_LIBS {"apollo-modules/apl-io/apl-io.bc", "apollo-modules/apl-strings/apl-string.bc"}

typedef struct LLVMComponents{
    LLVMContextRef ctx;
    LLVMBuilderRef builder;
    LLVMModuleRef module;
}LLVMComponents;





// initializes the required llvmlibs for LLVM to work.
void _apl_llvm_environment_setup(CodegenContext *context, LLVMComponents *components, ASTNode *program_node);
// shutsdown LLVM.
void _apl_llvm_shutdown(LLVMComponents *components);

void _apl_load_runtime_libraries(LLVMComponents *components);

//Import inbuilt runtime functions.
void _apl_import_runtime(LLVMModuleRef dest_module, const char *filename);

// saves the LLVM IR to a file and shutsdown LLVM.
void _apl_save_and_shutdown(LLVMComponents *components);


void _apl_gen_block_from_ast(LLVMComponents *components, CodegenContext *context, ASTNode *block_node);

void _apl_gen_function_start(LLVMComponents *components, CodegenContext *context, ASTNode *block_node);

void _apl_gen_function_end(LLVMComponents *components, CodegenContext *context, ASTNode *block_node);

void _apl_create_local_variable(LLVMComponents *components, CodegenContext *context, ASTNode *var_node);

DataType _apl_get_token_datatype(LLVMComponents *components, CodegenContext *context, ASTNode *token_node);

Symbol *get_token(CodegenContext *context, const char *name);

LLVMTypeRef _enquire_fxn_return_type(LLVMComponents *components,Fxn *fxn);

void _apl_gen_fxn_call_from_ast(LLVMComponents *components, CodegenContext *context, ASTNode *stmt);

void _apl_gen_println_ir(LLVMComponents *components, CodegenContext *context, Args *args);
void slice_string(Token string, char *clean_str);

float str_to_int_k(const char *s, int k);



#endif
