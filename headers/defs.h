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
#include <stdint.h>

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
typedef struct Arena Arena ;

#define EXPECTED_EXTENSION ".apl"
typedef bool set;

// generator
typedef struct CodegenContext {
  int symbol_count;
  int symbol_capacity;
  int string_constant_count;
  Symbol *symbols;
  Arena *a;
  SymbolTable *t;
} CodegenContext;


/**
 * Grows the symbol table array inside the CodegenContext if capacity is
 * reached.
 * @param context The codegen context containing the symbol table.
 */
void grow_symbols_if_needed(CodegenContext *context);


/**
 * Initializes the code generation context and opens the output file.
 * @param context The codegen context to initialize.
 * @param output_filename The target LLVM IR output file.
 */
void codegen_init(CodegenContext *context, const char *output_filename);


static void trace(const char *message);
#endif
