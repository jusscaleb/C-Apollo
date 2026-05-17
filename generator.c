#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "headers/token.h"


//intitialize the output file.

void codegen_init(CodegenContext* context, const char* output_filename){
    context->file = fopen(output_filename, "w");

    context->string_constant_count = 0;

    if(context->file == NULL){
        fprintf(stderr, "Could not create output file %s\n", output_filename);
        exit(1);

    }

    //Print headers and link C's native printf function for our standard I/O
    fprintf(context->file, "; --- Apollo Native Compiler Backend Output ---\n");

    fprintf(context->file, "declare i32 @printf(i8*, ...)\n\n" );
}

//Generate the opening main function.
void gen_function_start(CodegenContext* context, const char* name){
    //if it run then make that the main function.
    if(strcmp(name, "run") == 0){
        fprintf(context->file, "define i32 @main() {\n");
    }
    else{
        fprintf(context->file, "define void @%s()", name);
    }

}

//Translate println wrapper to a LLVM @printf call
void gen_println_statement(CodegenContext* context, const char* string_start, int length){
    int id = context->string_constant_count++;

    int internal_len = length - 2; //removing the quotes from the high level syntax. 
    int llvm_len = internal_len + 2; //include newline and null terminator.

    fprintf(context->file,"; Allocate string literal block in memory\n");

    // Copy the Apollo string literal into a local LLVM string buffer.
    fprintf(context->file, "%%str_%d = alloca [%d x i8]\n", id, llvm_len);
    fprintf(context->file, "store [%d x i8] c\"%.*s\\0A\\00\", [%d x i8]* %%str_%d\n", llvm_len, internal_len, string_start + 1, llvm_len, id);
    fprintf(context->file, "%%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* %%str_%d, i32 0, i32 0\n", id, llvm_len, llvm_len, id);

    //Call the printf operation
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%str_ptr_%d)\n\n", id);


}


//close function brackets safely
void gen_function_end(CodegenContext* context, bool is_main){
    (is_main) ? fprintf(context->file, "ret i32 0\n") : fprintf(context->file,  "    ret void\n");

    fprintf(context->file, "}\n");

}
