#ifndef FUNCTIONS
#define FUNCTIONS

#include <stdio.h>
#include <string.h>


typedef struct CodegenContext CodegenContext;

typedef enum{
    TYPE_STRING,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_NULL,
}returnType;


typedef struct{
    const char* name;
    returnType return_type;
    

}FXN;

void register_variable(CodegenContext *context, const char *name,
                       returnType return_type);

FXN *lookup_variable(CodegenContext *context, const char *name);

void create_fxn(CodegenContext *context, const char *number_start, int length,
                const char *name);



#endif