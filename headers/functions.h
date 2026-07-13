#ifndef FUNCTIONS
#define FUNCTIONS

#include "variables.h"
#include <stdio.h>
#include <string.h>

typedef struct CodegenContext CodegenContext;
typedef Fxn Fxn ;
typedef struct Params Params;

typedef struct Fxn{
  int length;
  int line;
  int level;
  DataType return_type;
  const char *name;
  struct Fxn *parent_fxn;
  Params *params;
} Fxn;



/**
 * Registers a new function in the symbol table.
 * @param context The codegen context containing the symbol table.
 * @param name The name of the function.
 * @param return_type The return type of the function.
 * @param level The nesting scope level of the function.
 * @param fxn The function structure containing metadata and parent links.
 */
Symbol* register_fxn(CodegenContext *context, const char *name,
                  DataType return_type, int level, Fxn *fxn);

#endif
