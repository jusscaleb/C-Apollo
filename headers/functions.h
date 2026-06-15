#ifndef FUNCTIONS
#define FUNCTIONS

#include "variables.h"
#include <stdio.h>
#include <string.h>

typedef struct CodegenContext CodegenContext;



typedef struct {
  const char *name;
  datatype return_type;
  int length;
  int line;
} FXN;
 
void register_fxn(CodegenContext *context, const char *name,
                  datatype return_type);

#endif