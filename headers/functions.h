#ifndef FUNCTIONS
#define FUNCTIONS

#include "variables.h"
#include <stdio.h>
#include <string.h>

typedef struct CodegenContext CodegenContext;


typedef struct {
  int length;
  int line;
  int level;
  datatype return_type;
  const char* fxn_name;
  const char *name;
} FXN;
 
void register_fxn(CodegenContext *context, const char *name,
                  datatype return_type, int level);

#endif