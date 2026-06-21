#ifndef DEFS_H
#define DEFS_H

#include "variables.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct errorStack errorStack;



#define EXPECTED_EXTENSION ".apl"
typedef bool set;

// generator
typedef struct CodegenContext {
  FILE *file;
  int symbol_count;
  int symbol_capacity;
  int string_constant_count;
  Symbol *symbols;
  int temp_count;

} CodegenContext;

char *realloc_space(char *value, int size);
void grow_symbols_if_needed(CodegenContext *context);
void free_codegen_context(CodegenContext *context, errorStack *s);
char *alloc_space(int num_elements, int element_size);

void codegen_init(CodegenContext *context, const char *output_filename);
void gen_function_start(CodegenContext *context, const char *name);
void gen_println_statement(CodegenContext *context, const char *string_start,
                           int length);
void gen_println_integer(CodegenContext *context, const char *number_start,
                         int length);
void gen_println_float(CodegenContext *context, const char *float_start,
                       int length);
void gen_function_end(CodegenContext *context, bool is_main);

#endif
