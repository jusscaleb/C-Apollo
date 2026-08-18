/*===================================================================
                          functions.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Functions Compile Time library.
      Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#ifndef FUNCTIONS
#define FUNCTIONS

#include "variables.h"

typedef struct CodegenContext CodegenContext;
typedef Fxn Fxn;
typedef struct Params Params;

typedef struct Fxn {
  int length;
  int line;
  int level;
  DataType return_type;
  const char *name;
  struct Fxn *parent_fxn;
  Params *params;
  int ret_nodes;
  int block_nodes;
} Fxn;

/**
 * Registers a new function in the symbol table.
 * @param context The codegen context containing the symbol table.
 * @param name The name of the function.
 * @param return_type The return type of the function.
 * @param level The nesting scope level of the function.
 * @param fxn The function structure containing metadata and parent links.
 */
Symbol *register_fxn(CodegenContext *context, const char *name,
                     DataType return_type, int level, Fxn *fxn, int len);

#endif
