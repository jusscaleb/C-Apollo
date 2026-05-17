#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>

#define EXPECTED_EXTENSION ".apl"
typedef bool set;


//generator

typedef struct {
    FILE* file;
    int string_constant_count;
} CodegenContext;

void codegen_init(CodegenContext* context, const char* output_filename);
void gen_function_start(CodegenContext* context, const char* name);
void gen_println_statement(CodegenContext* context, const char* string_start, int length);
void gen_println_integer(CodegenContext* context, const char* number_start, int length);
void gen_println_integer_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_float_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_mixed_addition(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
);

void gen_println_integer_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_float_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_mixed_multiplication(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
);

void gen_println_integer_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_float_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_mixed_subtraction(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
);

void gen_println_integer_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_float_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_mixed_division(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
);

void gen_println_integer_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_float_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length
);

void gen_println_mixed_modulus(
    CodegenContext* context,
    const char* left_start,
    int left_length,
    const char* right_start,
    int right_length,
    bool int_is_left
);



void gen_println_float(CodegenContext* context, const char* float_start, int length);
void gen_function_end(CodegenContext* context, bool is_main);



#endif
