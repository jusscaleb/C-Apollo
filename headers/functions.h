#ifndef FUNCTIONS
#define FUNCTIONS

#include "variables.h"
#include <stdio.h>
#include <string.h>

typedef struct CodegenContext CodegenContext;
typedef FXN FXN ;


typedef struct FXN{
  int length;
  int line;
  int level;
  datatype return_type;
  const char* name; //Name of the child of the parent_fxn
  struct FXN *parent_fxn;
} FXN;



/**
 * Registers a new function in the symbol table.
 * @param context The codegen context containing the symbol table.
 * @param name The name of the function.
 * @param return_type The return type of the function.
 * @param level The nesting scope level of the function.
 * @param fxn The function structure containing metadata and parent links.
 */
Symbol* register_fxn(CodegenContext *context, const char *name,
datatype return_type, int level, FXN *fxn);

#endif