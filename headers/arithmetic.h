#ifndef ARITHMETIC_H
#define ARITHMETIC_H

#include "defs.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>


// display either an integer or float as the final answer.
typedef enum {
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STRING,
    EXPR_BOOL
} ExprType;

// Stores the expression result
typedef struct ExprResult {
  ExprType type;
  char value[64]; // literal like "45" or LLVM temp like "%tmp_0"
  int str_len;    // For strings, holds the character length
} ExprResult;

/**
 * Creates an expression result from a literal token.
 * @param token The lexer token representing the literal.
 * @return An ExprResult containing the type and literal value string.
 */
ExprResult make_literal_expr(Token token);

/**
 * Generates LLVM IR for a binary arithmetic or comparison expression.
 * @param context The codegen context.
 * @param left The left operand expression result.
 * @param operator_type The token type of the binary operator (e.g., TOKEN_PLUS).
 * @param right The right operand expression result.
 * @return An ExprResult containing the LLVM temporary variable representing the computation result.
 */
ExprResult gen_binary_expr(CodegenContext *context, ExprResult left,
                           TokenType operator_type, ExprResult right);

/**
 * Generates LLVM IR to print an expression result to standard output.
 * @param context The codegen context.
 * @param result The expression result to print.
 */
void gen_println_expr(CodegenContext *context, ExprResult result);


#endif
