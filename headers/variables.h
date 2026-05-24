#ifndef VARIABLES
#define VARIABLES


#include <stdio.h>
#include "defs.h"


typedef enum{
    TYPE_STRING,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_FLOAT,
    TYPE_BOOL,
}datatype;


typedef struct{
    datatype type; //Token type
    const char* start; //RAM Address of Token
    int length; //
    int line;
}Variable;



void create_var(CodegenContext* context, const char* number_start, int length, datatype variable_type);

#endif

//static void store_var();