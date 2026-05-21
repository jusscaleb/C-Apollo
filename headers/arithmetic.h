#ifndef ARITHMETIC_H
#define ARITHMETIC_H


#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "token.h"

//display either an integer or float as the final answer.
typedef enum {
    EXPR_INT,
    EXPR_FLOAT
} ExprType;

//Stores the expression result
typedef struct {
    ExprType type;
    char value[64]; // literal like "45" or LLVM temp like "%tmp_0"
} ExprResult;


ExprResult make_literal_expr(Token token);
ExprResult gen_binary_expr(CodegenContext* context, ExprResult left, TokenType operator_type, ExprResult right);
void gen_println_expr(CodegenContext* context, ExprResult result);


#endif
