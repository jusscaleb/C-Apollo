/*===================================================================
                         llvm_backend.h

                    (c)2026 SCXRPIUS.dev

          LLVM API backend scaffold for Apollo code generation.
====================================================================*/

#ifndef APOLLO_LLVM_BACKEND_H
#define APOLLO_LLVM_BACKEND_H

#include "ast.h" 
#include "defs.h"
#include "token.h"
#include "variables.h"
#include "llvm-c/Analysis.h"
#include <llvm-c/Transforms/PassBuilder.h>

#include <llvm-c/Types.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <stdbool.h>
#include <stdint.h>

#define RUNTIME_LIBS                                                           \
  {"apollo-modules/apl-io/apl-io.bc",                                          \
   "apollo-modules/apl-string/apl-string.bc"}


#define VOID(x) (LLVMVoidTypeInContext(x))
#define I32(x)  (LLVMInt32TypeInContext(x))
#define F32(x)  (LLVMFloatTypeInContext(x))
#define I1(x)   (LLVMInt1TypeInContext(x))
#define I8(x)   (LLVMInt8TypeInContext(x))


typedef struct LLVMComponents {
  LLVMContextRef ctx;
  LLVMBuilderRef builder;
  LLVMModuleRef module;
  LLVMValueRef current_fxn;
} LLVMComponents;


// initializes the required llvmlibs for LLVM to work.
void _apl_llvm_environment_setup(
                                 LLVMComponents *components,
                                 ASTNode *program_node);
// shutsdown LLVM.
void _apl_llvm_shutdown(LLVMComponents *components);

void _apl_load_runtime_libraries(LLVMComponents *components);

// Import inbuilt runtime functions.
void _apl_import_runtime(LLVMModuleRef dest_module, const char *filename);

// saves the LLVM IR to a file and shutsdown LLVM.
void _apl_save_and_shutdown(LLVMComponents *components);

void _apl_gen_block_from_ast(LLVMComponents *components,
                              ASTNode *block_node);

void _apl_gen_function_start(LLVMComponents *components,
                              ASTNode *block_node);

void _apl_gen_function_end(LLVMComponents *components, 
                           ASTNode *block_node);

void _apl_create_local_variable(LLVMComponents *components,
                                 ASTNode *var_node);

DataType _apl_get_token_datatype(LLVMComponents *components,
                                  ASTNode *token_node);

void _apl_gen_fxn_call_from_ast(LLVMComponents *components,
                                 ASTNode *stmt);

void _apl_gen_println_ir(LLVMComponents *components, 
                         Args *args);
void slice_string(Token string, char *clean_str);

LLVMValueRef
load_variable(LLVMComponents *components, ASTNode *var_ref_node);

LLVMValueRef arihmetics(LLVMComponents *components, ASTNode *node,
                        char *result_name);

void _apl_reassign_variable(LLVMComponents *components, 
                            ASTNode *node);
void _apl_gen_if_block(LLVMComponents *components, 
                       ASTNode *node);
void _apl_gen_while_loop(LLVMComponents *components, 
                         ASTNode *node);

void _apl_gen_for_loop(LLVMComponents *components, 
                       ASTNode *node);

float str_to_int_k(const char *s, int k);

void  _apl_gen_return(LLVMComponents *components,  ASTNode *node);

float return_eval_int(ASTNode* expr);

LLVMValueRef _apl_eval_function_call(LLVMComponents *components, ASTNode *node);

uint32_t _apl_get_n_params(LLVMComponents *components, Params *p);

uint32_t _apl_get_n_args(LLVMComponents *components, Args *a);


LLVMTypeRef _apl_get_llvm_type(LLVMComponents*components, DataType dt);

void _apl_build_string_reassign(LLVMComponents *components, LLVMValueRef str_struct_ptr, char* char_ptr, LLVMTypeRef struct_type);

void _apl_optimize_module(LLVMComponents *components);


#endif
