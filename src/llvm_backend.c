#include <llvm-c/Core.h>
#include <stdio.h>
#include "../headers/llvm_backend.h"
#include "llvm-c/Types.h"


void llvm_environment_setup(CodegenContext *context, LLVMComponents *components, ASTNode *block_node) {
    LLVMContextRef ctx = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithNameInContext("apl_module", ctx);
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

    components->ctx = ctx;
    components->module = module;
    components->builder = builder;


    LLVMTypeRef main_fxn_type = LLVMFunctionType(LLVMVoidTypeInContext(ctx), NULL, 0, false);
    LLVMValueRef main_fxn = LLVMAddFunction(module, "main", main_fxn_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, main_fxn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMBuildRetVoid(builder);


    save_and_shutdown(components);
}



void save_and_shutdown(LLVMComponents *components){
    const int save_err = LLVMWriteBitcodeToFile(components->module, "temp\\output.bc");

    (save_err != 0) ? fprintf(stderr, "Error: Could not write LLVM IR to file\n") : 
    printf("LLVM IR successfully written to file\n");

    llvm_shutdown(components);
}


void llvm_shutdown(LLVMComponents *components){
    LLVMDisposeModule(components->module);
    LLVMDisposeBuilder(components->builder);
    LLVMContextDispose(components->ctx);
}
