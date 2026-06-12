#ifndef VARIABLES
#define VARIABLES

#include <stdio.h>
#include <string.h>

typedef struct CodegenContext CodegenContext;
typedef enum {
  TYPE_STRING,
  TYPE_INT,
  TYPE_CHAR,
  TYPE_FLOAT,
  TYPE_BOOL,
} datatype;

typedef struct {
  datatype type;     // Token type
  const char *start; // RAM Address of Token
  int length;
  int line;
} Variable;

typedef struct {
  char name[64];
  char llvm_name[64];
  datatype type;
  int str_length;
} Symbol;

void register_variable(CodegenContext *context, const char *name,
                       datatype variable_type);

Symbol *lookup_variable(CodegenContext *context, const char *name);

void create_var(CodegenContext *context, const char *number_start, int length,
                const char *name);

void gen_println_variable(CodegenContext *context, char *name);

#endif
