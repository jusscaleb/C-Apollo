/*=====================================================================
                              ast.h

                          (c)2026 SCXRPIUS.dev

              The Apollo ABSTRACT SYNTAX TREE Compile Time library.
             Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
------------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
=======================================================================*/


/*--------------------------------------------------------------------------------

                       ABSTACT SYNTAX TREE :)

---------------------------------------------------------------------------------*/

#ifndef AST_H
#define AST_H
#include "arithmetic.h"
#include "token.h"
#include "variables.h"
#include <stdio.h>

struct Symbol;


// Kind of nodes to expect
typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_BLOCK,

  AST_VAR_DECL,
  AST_PRINTLN,

  AST_LITERAL_EXPR,
  AST_BINARY_EXPR,

  AST_VAR_REF,

  AST_VAR_ASS,

  AST_CONCAT_STR,

  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_CALL_FXN,
  AST_RET_NODE,
} ASTNodeType;

typedef struct ASTNode ASTNode;
typedef struct Params Params;
typedef struct Args Args;

typedef struct Params{
  ASTNode* param;
  struct Params* next;
}Params;

typedef struct Args{
  ASTNode* arg;
  struct Args* next;
}Args;


// Stores the specified Node in the AST.
struct ASTNode {
  ASTNodeType Type;

  union {
    struct {
      ASTNode *function;
      int line;
      int column;

    } program;

    struct {
      const char *name;
      int name_length;
      ASTNode *body;
      int level;
      Fxn fxn;
      Symbol *resolved_symbol;
      int line;
      int column;
    } function;

    struct {
      ASTNode **statements;
      int count;
      int capacity;
      int line;
      int column;

    } block;

    struct {
      const char *name;
      int name_length;
      DataType value_type;
      ASTNode *value;
      Fxn fxn;
      int level;
      int line;
      int column;

    } var_decl;

    struct {
      ASTNode *value;
      int line;
      int column;

    } println;

    struct {
      Token token;
      int line;
      int column;
    } literal_expr;

    struct {
      ASTNode *left;
      TokenType operator_type;
      ASTNode *right;
      int line;
      int column;
    } binary_expr;

    struct {
      const char *name;
      int name_length;
      Fxn fxn;
      int level;
      struct Symbol *resolved_symbol;
      int line;
      int column;
    } var_ref;

    struct {
      const char *name;
      int name_length;
      ASTNode *value;
      Fxn fxn;
      int level;
      struct Symbol *resolved_symbol;
      int line;
      int column;
    } var_assign;

    struct {
      ASTNode *left;
      ASTNode *right;
      int line;
      int column;

    } str_concat;

    struct {
      ASTNode *condition;
      ASTNode *then_block;
      ASTNode *else_block;
      int line;
      int column;
    } if_stmt;

    struct {
      ASTNode *condition;
      ASTNode *then_block;
      int line;
      int column;
    } while_lp;

    struct {
      ASTNode *variable;
      ASTNode *condtion;
      ASTNode *var_operation;
      ASTNode *then_block;
      int line;
      int column;
    } for_lp;

    struct {
      const char *name;
      int name_length;
      DataType return_type;
      Fxn fxn;
      int level;
      Symbol *resolved_symbol;
      Args *args;
      int line;
      int column;
    } call_fxn;

    struct {
      ASTNode* value;
      Fxn fxn;
      int line;
      int column;
    }ret_node;
  };

  
};

/*--------------------------------------------------------------------------------

                       NODE CREATION -> BEGIN

---------------------------------------------------------------------------------*/

/**
 * Creates an AST node representing a literal value.
 * @param token The Token representing the literal (number, string, bool, null).
 */
ASTNode *create_literal_node(Token token);

ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right, int line, int column);

ASTNode *create_var_decl_node(const char *name, int name_length,
                              DataType value_type, ASTNode *value, Fxn fxn,
                              int level, int line, int column);

ASTNode *create_println_node(ASTNode *value, int line, int column);

ASTNode *create_block_node(int line, int column);

ASTNode *create_var_ref_node(const char *name, int name_length, Fxn fxn,
                             int level, int line, int column);

ASTNode *create_var_assign_node(const char *name, int name_length,
                                ASTNode *value, Fxn fxn, int level, int line, int column);

ASTNode *create_concat_node(ASTNode *left, ASTNode *right, int line, int column);

ASTNode *create_if_node(ASTNode *condition, ASTNode *then_block,
                        ASTNode *else_block, int line, int column);

ASTNode *create_while_node(ASTNode *condition, ASTNode *then_block, int line, int column);

ASTNode *create_for_node(ASTNode *variable, ASTNode *condition,
                         ASTNode *var_operation, ASTNode *then_block, int line, int column);

ASTNode *create_fxn_call_node(char *name, int name_length, DataType return_type,
                              Fxn fxn, int level, Args *args, int line, int column);

ASTNode *create_function_node(const char *name, int name_length, ASTNode *body,
                              Fxn fxn, int level, int line, int column);

ASTNode *create_program_node(ASTNode *block);

ASTNode *create_ret_node(CodegenContext *context, Fxn fxn, ASTNode *value, int line, int column);

/*--------------------------------------------------------------------------------

                       NODE CREATION -> END

---------------------------------------------------------------------------------*/


/**
 * Realloc more memory to the AST block
 * @param block The AST node representing the block to add.
 * @param statement The type of statement the block falls under.
 */
void block_add_statement(ASTNode *block, ASTNode *statement);

/**
 * Generates LLVM IR for a println statement.
 * @param context The codegen context containing output file and state.
 * @param println_node The AST node representing the println statement.
 */
void gen_println_from_ast(CodegenContext *context, ASTNode *println_node);

/**
 * Generates LLVM IR for an expression and returns its result type/value.
 * @param context The codegen context.
 * @param expr The AST node representing the expression.
 * @return ExprResult containing the LLVM value string and type.
 */
ExprResult gen_expr_from_ast(CodegenContext *context, ASTNode *expr);

/**
 * Generates LLVM IR for a variable declaration.
 * @param context The codegen context.
 * @param var_node The AST node representing the variable declaration.
 */
void gen_var_decl_from_ast(CodegenContext *context, ASTNode *var_node);

/**
 * Generates LLVM IR for a variable reassignment.
 * @param context The codegen context.
 * @param assign_node The AST node representing the reassignment.
 */
void gen_var_assign_from_ast(CodegenContext *context, ASTNode *assign_node);

/**
 * Generates LLVM IR for an if/elif/else branching structure.
 * @param context The codegen context.
 * @param if_node The AST node representing the conditional block.
 */
void gen_if_from_ast(CodegenContext *context, ASTNode *if_node);

/**
 * Generates LLVM IR for a block of statements.
 * @param context The codegen context.
 * @param block_node The AST block node containing statements to generate.
 */
void gen_block_from_ast(CodegenContext *context, ASTNode *block_node);

/**
 * Generates LLVM IR for a while loop.
 * @param context The codegen context.
 * @param while_node The AST node representing the while loop.
 */
void gen_while_from_ast(CodegenContext *context, ASTNode *while_node);

/**
 * Generates LLVM IR for a for loop.
 * @param context The codegen context.
 * @param for_node The AST node representing the for loop.
 */
void gen_for_from_ast(CodegenContext *context, ASTNode *for_node);

/**
 * Generates LLVM IR for a function call.
 * @param context The codegen context.
 * @param call_node The AST node representing the function call.
 */
void gen_fxn_call_from_ast(CodegenContext *context, ASTNode *call_node);



/**
 * Generates the entry point and LLVM IR for the entire program.
 * @param context The codegen context.
 * @param program_node The root AST program node.
 */
void gen_program_from_ast(CodegenContext *context, ASTNode *program_node);

#endif
