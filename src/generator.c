#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../headers/token.h"


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

static int escaped_string_byte_count(const char* string_start, int length){
    int count = 0;

    for(int i = 0; i < length; i++){
        if(string_start[i] == '\\' && i + 1 < length){
            i++;
        }

        count++;
    }

    return count;
}


//Replaces escape characters with llvm format contents.
static void emit_llvm_string_contents(FILE* file, const char* string_start, int length){
    for(int i = 0; i < length; i++){
        if(string_start[i] == '\\' && i + 1 < length){
            i++;

            switch(string_start[i]){
                case 'n':
                    fprintf(file, "\\0A");
                    break;
                case 't':
                    fprintf(file, "\\09");
                    break;
                case '"':
                    fprintf(file, "\\22");
                    break;
                case '\\':
                    fprintf(file, "\\5C");
                    break;
                default:
                    fprintf(file, "%c", string_start[i]);
                    break;
            }

            continue;
        }

        switch(string_start[i]){
            case '"':
                fprintf(file, "\\22");
                break;
            case '\\':
                fprintf(file, "\\5C");
                break;
            default:
                fprintf(file, "%c", string_start[i]);
                break;
        }
    }
}

//Translate println wrapper to a LLVM @printf call
void gen_println_statement(CodegenContext* context, const char* string_start, int length){
    int id = context->string_constant_count++;
    int internal_len = length - 1; //removing the quotes from the high level syntax. 
    int runtime_len = escaped_string_byte_count(string_start + 1, internal_len);
    int llvm_len = runtime_len + 2; //include newline and null terminator.
    fprintf(context->file,"; Allocate string literal block in memory\n");
    // Copy the Apollo string literal into a local LLVM string buffer.
    fprintf(context->file, "%%str_%d = alloca [%d x i8]\n", id, llvm_len);
    fprintf(context->file, "store [%d x i8] c\"", llvm_len);
    emit_llvm_string_contents(context->file, string_start + 1, internal_len);
    fprintf(context->file, "\\0A\\00\", [%d x i8]* %%str_%d\n", llvm_len, id);
    fprintf(context->file, "%%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* %%str_%d, i32 0, i32 0\n", id, llvm_len, llvm_len, id);

    //Call the printf operation
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%str_ptr_%d)\n\n", id);


}

void gen_println_integer(CodegenContext* context, const char* number_start, int length){
    int id = context->string_constant_count++;

    fprintf(context->file, "; Allocate integer printf format block in memory\n");
    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %.*s)\n\n", id, length, number_start);
}

void gen_println_float(CodegenContext* context, const char* float_start, int length){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %.*s)\n\n", id, length, float_start);
}


/*----------ARITHMETIC OPERATIONS-----------*/

//Adds two integers.
void gen_println_integer_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%add_%d = add i32 %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %%add_%d)\n\n", id, id);
}

//Adds two floats.
void gen_println_float_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%add_%d = fadd double %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%add_%d)\n\n", id, id);
}

//Add mixed values
void gen_println_mixed_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
){
    int id = context->string_constant_count++;

    if (int_is_left) {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                left_length,
                left_start);

        fprintf(context->file, "%%add_%d = fadd double %%int_to_double_%d, %.*s\n",
                id,
                id,
                right_length,
                right_start);
    } else {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                right_length,
                right_start);

        fprintf(context->file, "%%add_%d = fadd double %.*s, %%int_to_double_%d\n",
                id,
                left_length,
                left_start,
                id);
    }

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%add_%d)\n\n", id, id);
}


//Multiplication
void gen_println_integer_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%mul_%d = mul i32 %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %%mul_%d)\n\n", id, id);
}


//Float Multiplication
void gen_println_float_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%mul_%d = fmul double %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%mul_%d)\n\n", id, id);
}


//Mixed Multiplication
void gen_println_mixed_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
){
    int id = context->string_constant_count++;

    if (int_is_left) {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                left_length,
                left_start);

        fprintf(context->file, "%%mul_%d = fmul double %%int_to_double_%d, %.*s\n",
                id,
                id,
                right_length,
                right_start);
    } else {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                right_length,
                right_start);

        fprintf(context->file, "%%mul_%d = fmul double %.*s, %%int_to_double_%d\n",
                id,
                left_length,
                left_start,
                id);
    }

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%mul_%d)\n\n", id, id);
}

//Integer Subtraction
void gen_println_integer_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%sub_%d = sub i32 %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %%sub_%d)\n\n", id, id);
}

//Float Subtraction
void gen_println_float_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%sub_%d = fsub double %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%sub_%d)\n\n", id, id);
}

//Mixed Subtraction
void gen_println_mixed_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
){
    int id = context->string_constant_count++;

    if (int_is_left) {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                left_length,
                left_start);

        fprintf(context->file, "%%sub_%d = fsub double %%int_to_double_%d, %.*s\n",
                id,
                id,
                right_length,
                right_start);
    } else {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                right_length,
                right_start);

        fprintf(context->file, "%%sub_%d = fsub double %.*s, %%int_to_double_%d\n",
                id,
                left_length,
                left_start,
                id);
    }

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%sub_%d)\n\n", id, id);
}

//Integer Division
void gen_println_integer_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%div_%d = sdiv i32 %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %%div_%d)\n\n", id, id);
}

//Float Division
void gen_println_float_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%div_%d = fdiv double %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%div_%d)\n\n", id, id);
}

//Mixed Division
void gen_println_mixed_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
){
    int id = context->string_constant_count++;

    if (int_is_left) {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                left_length,
                left_start);

        fprintf(context->file, "%%div_%d = fdiv double %%int_to_double_%d, %.*s\n",
                id,
                id,
                right_length,
                right_start);
    } else {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                right_length,
                right_start);

        fprintf(context->file, "%%div_%d = fdiv double %.*s, %%int_to_double_%d\n",
                id,
                left_length,
                left_start,
                id);
    }

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%div_%d)\n\n", id, id);
}

//Integer Modulus
void gen_println_integer_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%mod_%d = srem i32 %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %%mod_%d)\n\n", id, id);
}

//Float Modulus
void gen_println_float_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%mod_%d = frem double %.*s, %.*s\n",
            id,
            left_length, left_start,
            right_length, right_start);

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%mod_%d)\n\n", id, id);
}

//Mixed Modulus
void gen_println_mixed_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
){
    int id = context->string_constant_count++;

    if (int_is_left) {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                left_length,
                left_start);

        fprintf(context->file, "%%mod_%d = frem double %%int_to_double_%d, %.*s\n",
                id,
                id,
                right_length,
                right_start);
    } else {
        fprintf(context->file, "%%int_to_double_%d = sitofp i32 %.*s to double\n",
                id,
                right_length,
                right_start);

        fprintf(context->file, "%%mod_%d = frem double %.*s, %%int_to_double_%d\n",
                id,
                left_length,
                left_start,
                id);
    }

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %%mod_%d)\n\n", id, id);
}


/*------------END OF ENTIRE PROGRAM------------*/

//add the last curly bracket.
void gen_function_end(CodegenContext* context, bool is_main){
    (is_main) ? fprintf(context->file, "ret i32 0\n") : fprintf(context->file,  "    ret void\n");

    fprintf(context->file, "}\n");
    fclose(context->file);

}
