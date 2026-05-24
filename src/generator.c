#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../headers/token.h"
#include "../headers/variables.h"
#include "../headers/arithmetic.h"


// Opens the LLVM output file and resets the counters used for generated names.
void codegen_init(CodegenContext* context, const char* output_filename){
    context->file = fopen(output_filename, "w");

    context->string_constant_count = 0;
    context->temp_count = 0;

    if(context->file == NULL){
        fprintf(stderr, "Could not create output file %s\n", output_filename);
        exit(1);

    }

    //Print headers and link C's native printf function for our standard I/O
    fprintf(context->file, "; --- Apollo Native Compiler Backend Output ---\n");

    fprintf(context->file, "declare i32 @printf(i8*, ...)\n\n" );
}

// Emits the LLVM function header. Apollo's run() function becomes LLVM's main().
void gen_function_start(CodegenContext* context, const char* name){
    //if it run then make that the main function.
    if(strcmp(name, "run") == 0){
        fprintf(context->file, "define i32 @main() {\n");
    }
    else{
        fprintf(context->file, "define void @%s()", name);
    }

}

// Counts how many runtime bytes a string literal needs after escape sequences
// like \n, \t, \", and \\ are converted into one actual character.
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


// Writes string contents in LLVM's escaped format.
// Example: a newline in Apollo becomes \0A in the generated LLVM string.
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

// Emits LLVM that prints an Apollo string literal with printf.
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

// Emits LLVM that prints a single integer literal.
void gen_println_integer(CodegenContext* context, const char* number_start, int length){
    int id = context->string_constant_count++;

    fprintf(context->file, "; Allocate integer printf format block in memory\n");
    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %.*s)\n\n", id, length, number_start);
}

// Emits LLVM that prints a single decimal literal as a double.
void gen_println_float(CodegenContext* context, const char* float_start, int length){
    int id = context->string_constant_count++;

    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %.*s)\n\n", id, length, float_start);
}

/*------------VARIABLES------------*/

static const char* llvm_datatype(datatype type){
    switch(type){
        case TYPE_INT:
            return "i32";

        case TYPE_FLOAT:
            return "double";

        case TYPE_BOOL:
            return "i1";

        case TYPE_CHAR:
            return "i8";

        default:
            return NULL;
        
    }
}



// Emits LLVM for an integer variable declaration.
// Example: var age = 45; becomes an alloca slot plus a store into that slot.
void create_var(CodegenContext *context, const char *number_start, int length, datatype variable_type){
    int id = context->string_constant_count++;

    fprintf(context->file, "; Allocate integer variable slot\n");

    const char* llvm_type = llvm_datatype(variable_type);
   if(llvm_type == NULL){
    char string_type[32];
    snprintf(string_type, sizeof(string_type), "[%d x i8]", length);
    fprintf(context->file, "%%var_%d = alloca %s\n", id, string_type);
    fprintf(context->file, "store %s c\"%.*s\\00\", %s* %%var_%d\n\n", string_type, length - 1, number_start + 1, string_type, id);


}else{
    fprintf(context->file, "%%var_%d = alloca %s\n", id, llvm_type);
    fprintf(context->file, "store %s %.*s, %s* %%var_%d\n\n", llvm_type,length, number_start, llvm_type, id);

}




}


/*------------ARITHMETICS------------*/
// Converts a lexer token into an expression result.
// The value is kept as text so it can be written directly into LLVM later.
ExprResult make_literal_expr(Token token){
    ExprResult result;

    if(token.type == TOKEN_FLOAT){
        result.type = EXPR_FLOAT;
    }
    else{
        result.type = EXPR_INT;
    }

    snprintf(result.value, sizeof(result.value), "%.*s", token.length, token.start);

    return result;
}

// Emits LLVM for a binary arithmetic expression such as left + right.
// Returns a new ExprResult pointing at the generated temporary value.
ExprResult gen_binary_expr(CodegenContext* context, ExprResult left, TokenType operator_type, ExprResult right){
    ExprResult result;
    int id = context->temp_count++;

    // If either side is a decimal, the whole operation must use LLVM double math.
    bool use_float = left.type == EXPR_FLOAT || right.type == EXPR_FLOAT;

    if(use_float){
        char left_value[64];
        char right_value[64];

        // Convert integer operands to double before mixed int/float arithmetic.
        if(left.type == EXPR_INT){
            int cast_id = context->temp_count++;
            fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id, left.value);
            snprintf(left_value, sizeof(left_value), "%%tmp_%d", cast_id);
        }
        else{
            snprintf(left_value, sizeof(left_value), "%s", left.value);
        }

        // Convert the right side too if it is the integer part of a mixed expression.
        if(right.type == EXPR_INT){
            int cast_id = context->temp_count++;
            fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id, right.value);
            snprintf(right_value, sizeof(right_value), "%%tmp_%d", cast_id);
        }
        else{
            snprintf(right_value, sizeof(right_value), "%s", right.value);
        }

        // Choose the LLVM floating-point instruction for this Apollo operator.
        const char* llvm_op = NULL;

        switch(operator_type){
            case TOKEN_ADD: llvm_op = "fadd"; break;
            case TOKEN_SUB: llvm_op = "fsub"; break;
            case TOKEN_MUL: llvm_op = "fmul"; break;
            case TOKEN_DIV: llvm_op = "fdiv"; break;
            case TOKEN_MOD: llvm_op = "frem"; break;
            default:
                fprintf(stderr, "Invalid float operator.\n");
                exit(1);
        }

        fprintf(context->file, "%%tmp_%d = %s double %s, %s\n", id, llvm_op, left_value, right_value);

        result.type = EXPR_FLOAT;
        snprintf(result.value, sizeof(result.value), "%%tmp_%d", id);

        return result;
    }

    // If both sides are integers, use LLVM integer arithmetic instructions.
    const char* llvm_op = NULL;

    switch(operator_type){
        case TOKEN_ADD: llvm_op = "add"; break;
        case TOKEN_SUB: llvm_op = "sub"; break;
        case TOKEN_MUL: llvm_op = "mul"; break;
        case TOKEN_DIV: llvm_op = "sdiv"; break;
        case TOKEN_MOD: llvm_op = "srem"; break;
        default:
            fprintf(stderr, "Invalid integer operator.\n");
            exit(1);
    }

    fprintf(context->file, "%%tmp_%d = %s i32 %s, %s\n", id, llvm_op, left.value, right.value);

    result.type = EXPR_INT;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", id);

    return result;
}

// Emits LLVM that prints the final result of a parsed expression.
// The ExprResult type decides whether printf receives an i32 or a double.
void gen_println_expr(CodegenContext* context, ExprResult result){
    int id = context->string_constant_count++;

    if(result.type == EXPR_FLOAT){
        fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
        fprintf(context->file, "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
        fprintf(context->file, "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%float_fmt_%d, i32 0, i32 0\n", id, id);
        fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %s)\n\n", id, result.value);
        return;
    }

    fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file, "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
    fprintf(context->file, "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* %%int_fmt_%d, i32 0, i32 0\n", id, id);
    fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %s)\n\n", id, result.value);
}
/*------------END OF ENTIRE PROGRAM------------*/

// Emits the return instruction, closes the current LLVM function, and closes the file.
void gen_function_end(CodegenContext* context, bool is_main){
    (is_main) ? fprintf(context->file, "ret i32 0\n") : fprintf(context->file,  "    ret void\n");

    fprintf(context->file, "}\n");
    fclose(context->file);

}
