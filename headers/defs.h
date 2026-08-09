/*===================================================================
                              defs.h

                        (c)2026 SCXRPIUS.dev

              The Apollo general definitions Compile Time library.
             Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#ifndef DEFS_H
#define DEFS_H

#include "variables.h"

#include <stdbool.h>
#include <stdio.h>


#define _DB 0
#define _DEBUG(prompt)                                                         \
  do {                                                                         \
    if (_DB)                                                                   \
      printf("%s\n", prompt);                                                  \
  } while (0);



typedef struct ErrorStack ErrorStack;
typedef struct ASTNode ASTNode;
typedef struct ApolloLLVMBackend ApolloLLVMBackend;

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
  ASTNode **deferred_functions;
  int deferred_count;
  int deferred_capacity;
} CodegenContext;

/**
 * Reallocates memory space for a string, ensuring null termination.
 * @param value The existing string pointer.
 * @param size The new size required.
 * @return A pointer to the newly reallocated string.
 */
char *realloc_space(char *value, int size);

/**
 * Grows the symbol table array inside the CodegenContext if capacity is
 * reached.
 * @param context The codegen context containing the symbol table.
 */
void grow_symbols_if_needed(CodegenContext *context);

/**
 * Frees all memory associated with the code generation context and error stack.
 * @param context The codegen context.
 * @param s The error stack.
 */
void free_codegen_context(CodegenContext *context, ErrorStack *s);

/**
 * Allocates memory initialized to zero.
 * @param num_elements Number of elements to allocate.
 * @param element_size Size of each element.
 * @return A pointer to the allocated memory.
 */
char *alloc_space(int num_elements, int element_size);

/**
 * Initializes the code generation context and opens the output file.
 * @param context The codegen context to initialize.
 * @param output_filename The target LLVM IR output file.
 */
void codegen_init(CodegenContext *context, const char *output_filename);

/**
 * Initializes the LLVM backend scaffolding used for API-driven codegen.
 * The implementation is intentionally isolated from the current text emitter.
 */
bool llvm_backend_init(ApolloLLVMBackend *backend, const char *module_name,
                       const char *output_path);

/**
 * Releases any LLVM backend resources allocated by llvm_backend_init.
 */
void llvm_backend_dispose(ApolloLLVMBackend *backend);

/**
 * Emits LLVM IR to define the start of a function.
 * @param context The codegen context.
 * @param name The name of the function.
 */
void gen_function_start(CodegenContext *context, ASTNode *func_node);

/**
 * Emits LLVM IR to print a string literal.
 * @param context The codegen context.
 * @param string_start Pointer to the start of the string literal token.
 * @param length Length of the string literal.
 */
void gen_println_statement(CodegenContext *context, const char *string_start,
                           int length);

/**
 * Emits LLVM IR to print an integer literal.
 * @param context The codegen context.
 * @param number_start Pointer to the start of the integer literal token.
 * @param length Length of the integer literal.
 */
void gen_println_integer(CodegenContext *context, const char *number_start,
                         int length);

/**
 * Emits LLVM IR to print a float literal.
 * @param context The codegen context.
 * @param float_start Pointer to the start of the float literal token.
 * @param length Length of the float literal.
 */
void gen_println_float(CodegenContext *context, const char *float_start,
                       int length);

/**
 * Emits LLVM IR to close a function block.
 * @param context The codegen context.
 * @param is_main Boolean flag indicating if this is the program entry point.
 */
void gen_function_end(CodegenContext *context, bool is_main,
                      DataType return_type);

#endif
