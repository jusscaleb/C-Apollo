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



void register_fxn(CodegenContext *context, const char *name,
                  datatype return_type, int level);

#endif