/*===================================================================
                         llvm_backend.h

                    (c)2026 SCXRPIUS.dev

          LLVM API backend scaffold for Apollo code generation.
====================================================================*/

#ifndef APOLLO_LLVM_BACKEND_H
#define APOLLO_LLVM_BACKEND_H

#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include "ast.h"
#include "defs.h"



typedef struct LLVMComponents{
    LLVMContextRef ctx;
    LLVMBuilderRef builder;
    LLVMModuleRef module;
}LLVMComponents;



// initializes the required llvmlibs for LLVM to work.
void llvm_environment_setup(CodegenContext *context, LLVMComponents *components, ASTNode *program_node);
// shutsdown LLVM.
void llvm_shutdown(LLVMComponents *components);

// saves the LLVM IR to a file and shutsdown LLVM.
void save_and_shutdown(LLVMComponents *components);

// generates the println from the AST node.
void gen_println_ir(LLVMComponents *components, CodegenContext *context, ASTNode* block_node);



#endif
