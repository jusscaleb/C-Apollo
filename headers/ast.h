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

    } program;

    struct {
      const char *name;
      int name_length;
      ASTNode *body;
      int level;
      Fxn fxn;
      Symbol *resolved_symbol;
    } function;

    struct {
      ASTNode **statements;
      int count;
      int capacity;

    } block;

    struct {
      const char *name;
      int name_length;
      DataType value_type;
      ASTNode *value;
      Fxn fxn;
      int level;

    } var_decl;

    struct {
      ASTNode *value;

    } println;

    struct {
      Token token;

    } literal_expr;

    struct {
      ASTNode *left;
      TokenType operator_type;
      ASTNode *right;
    } binary_expr;

    struct {
      const char *name;
      int name_length;
      Fxn fxn;
      int level;
      struct Symbol *resolved_symbol;
    } var_ref;

    struct {
      const char *name;
      int name_length;
      ASTNode *value;
      Fxn fxn;
      int level;
      struct Symbol *resolved_symbol;
    } var_assign;

    struct {
      ASTNode *left;
      ASTNode *right;

    } str_concat;

    struct {
      ASTNode *condition;
      ASTNode *then_block;
      ASTNode *else_block;
    } if_stmt;

    struct {
      ASTNode *condition;
      ASTNode *then_block;
    } while_lp;

    struct {
      ASTNode *variable;
      ASTNode *condtion;
      ASTNode *var_operation;
      ASTNode *then_block;

    } for_lp;

    struct {
      const char *name;
      int name_length;
      DataType return_type;
      Fxn fxn;
      int level;
      Symbol *resolved_symbol;
      Args *args;
    } call_fxn;
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

/**
 * Creates an AST node representing a binary operation.
 * @param left The AST node for the left operand.
 * @param operator_type The token operator type (e.g. TOKEN_ADD, TOKEN_AND).
 * @param right The AST node for the right operand.
 */
ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right);

/**
 * Creates an AST node representing a variable declaration.
 * @param name Pointer to the variable identifier character sequence.
 * @param name_length Length of the variable name identifier.
 * @param value_type The compiler DataType of the variable.
 * @param value The AST node representing the initial value expression.
 * @param fxn_name fxn the variable belongs to.
 * @param level scope level of the variable.
 */
ASTNode *create_var_decl_node(const char *name, int name_length,
                              DataType value_type, ASTNode *value, Fxn fxn,
                              int level);

/**
 * Creates an AST node representing a println statement.
 * @param value The AST node representing the expression to print.
 */
ASTNode *create_println_node(ASTNode *value);

/**
 * Creates an empty AST block container to hold statements.
 */
ASTNode *create_block_node();

/**
 * Creates an AST node representing a reference to a variable by name.
 * @param name Pointer to the variable identifier.
 * @param name_length Length of the variable name.
 * @param fxn_name fxn the variable belongs to.
 * @param level scope level of the variable.
 */
ASTNode *create_var_ref_node(const char *name, int name_length, Fxn fxn,
                             int level);

/**
 * Creates an AST node representing a variable reassignment.
 * @param name The LLVM-compatible name of the variable.
 * @param name_length Length of the variable's LLVM name.
 * @param value The AST node representing the new value expression.
 * @param fxn_name fxn the variable belongs to.
 * @param level scope level of the variable.
 */
ASTNode *create_var_assign_node(const char *name, int name_length,
                                ASTNode *value, Fxn fxn, int level);

/**
 * Creates an AST node representing a string concatenation.
 * @param left The AST node of the left-hand string expression.
 * @param right The AST node of the right-hand string expression.
 */
ASTNode *create_concat_node(ASTNode *left, ASTNode *right);

/**
 * Creates an AST node representing conditional branching (if/elif/else).
 * @param condition The AST node evaluating to the boolean condition.
 * @param then_block The AST block to execute if the condition is true.
 * @param else_block The AST block or next conditional node if the condition is
 * false.
 */
ASTNode *create_if_node(ASTNode *condition, ASTNode *then_block,
                        ASTNode *else_block);

/**
 * Creates an AST node representing a while loop.
 * @param condition The AST node evaluating to the loop condition.
 * @param then_block The AST block to execute repeatedly while condition is
 * true.
 */
ASTNode *create_while_node(ASTNode *condition, ASTNode *then_block);

/**
 * Creates an AST node representing a for loop.
 * @param variable The AST node representing loop variable initialization.
 * @param condition The AST node representing loop continuation condition.
 * @param var_operation The AST node representing loop variable step/update.
 */
ASTNode *create_for_node(ASTNode *variable, ASTNode *condition,
                         ASTNode *var_operation, ASTNode *then_block);

/**
 * Creates an AST node representing fxn call.
 * @param name The AST node representing the name of the fxn.
 * @param name_length The AST node representing the length of the name.
 * @param return_type The AST node representing the return type of the fxn.
 */
ASTNode *create_fxn_call_node(char *name, int name_length, DataType return_type,
                              Fxn fxn, int level);

/*--------------------------------------------------------------------------------

                       NODE CREATION -> END

---------------------------------------------------------------------------------*/

/**
 * Creates an AST node representing a function definition.
 * @param name The name of the function.
 * @param name_length Length of the function name.
 * @param body The AST block node representing the function body.
 */
ASTNode *create_function_node(const char *name, int name_length, ASTNode *body,
                              Fxn fxn, int level);

/**
 * Creates an AST node representing the entire program.
 * @param block The block containing top-level declarations/functions.
 */
ASTNode *create_program_node(ASTNode *block);

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
