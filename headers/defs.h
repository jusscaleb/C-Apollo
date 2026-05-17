#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdbool.h>

typedef bool set;


//generator

typedef struct {
    FILE* file;
    int string_constant_count;
} CodegenContext;

void codegen_init(CodegenContext* context, const char* output_filename);
void gen_function_start(CodegenContext* context, const char* name);
void gen_println_statement(CodegenContext* context, const char* string_start, int length);
void gen_function_end(CodegenContext* context, bool is_main);



#endif