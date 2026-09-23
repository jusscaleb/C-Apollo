/*=====================================================================
                              ast.h

                          (c)2026 SCXRPIUS.dev

              The Apollo ABSTRACT SYNTAX TREE Compile Time library.
             Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

------------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
=======================================================================*/

#ifndef AST_H
#define AST_H
#include "../headers/memory.h"
#include "token.h"
#include "variables.h"
#include <stdint.h>

struct Symbol;

// Kind of nodes to expect
typedef enum {
  AST_PROGRAM,
  AST_FUNCTION,
  AST_BLOCK,

  AST_VAR_DECL,
  AST_LITERAL_EXPR,
  AST_BINARY_EXPR,
  AST_URINARY_EXPR,

  AST_VAR_REF,

  AST_VAR_ASS,

  AST_IF,
  AST_WHILE,
  AST_FOR,
  AST_CALL_FXN,
  AST_RET_NODE,
  AST_ARRAY_LITERAL,
  AST_INDEX_EXPR,
  AST_STRUCT_DEFINITION,
  AST_STRUCT_DECL,
  AST_ACCESS,
  AST_FUNCTIONS,
} ASTNodeType;

typedef struct ASTNode ASTNode;
typedef struct Params Params;
typedef struct Args Args;

typedef struct Params {
  ASTNode *param;
  struct Params *next;
} Params;

typedef struct Struct {
  ASTNode *field;
  struct Struct *next;
} Struct;

typedef struct Args {
  ASTNode *arg;
  DataType datatype;
  struct Args *next;
} Args;

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
      Fxns fxns;
      Fxn fxn;
      Symbol *resolved_symbol;
      int pointer_level;
    } function;

    struct {
      char *name;
      int name_length;
      struct Symbol *resolved_symbol;
      int level;
      int count;
      int capacity;
      ASTNode **methods;
      Fxns *fxns;
    } functions;

    struct {
      ASTNode **statements;
      int count;
      int capacity;
      int level;
    } block;

    struct {
      const char *name;
      int name_length;
      DataType value_type;
      const char *struct_type_name;
      int struct_type_name_len;
      ASTNode *value;
      Fxn fxn;
      int level;
      int pointer_level;
      struct Symbol *resolved_symbol;
      MemoryBucket bucket;
      int array_count;
    } var_decl;

    struct {
      Token token;
    } literal_expr;

    struct {
      ASTNode *left;
      TokenType operator_type;
      ASTNode *right;
      int line;
      int column;
    } binary_expr;

    struct {
      TokenType operator_type;
      ASTNode *value;
      DataType eval_type;
    } urinary_expr;

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
      int deref_level;
      ASTNode *target_node;
    } var_assign;

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
      ASTNode *value;
      Fxn fxn;
    } ret_node;

    struct {
      ASTNode **elements;
      uint32_t count;
      uint32_t capacity;
      DataType element_type;
      bool is_dynamic;
    } array_literal;

    struct {
      ASTNode *target;
      ASTNode *index;
      DataType type;
    } index_expr;

    struct {
      Symbol *sym;
      Struct *fields;
      Fxn *fxn;
      int level;
      MemoryBucket bucket;
      char *name;
      int name_length;
    } struct_expr;

    struct {
      ASTNode *src;
      ASTNode *target;
      DataType datatype;
      char *struct_type_name;
      Struct *fields;
    } access;
  };

  
};

/*--------------------------------------------------------------------------------

                       NODE CREATION -> BEGIN

---------------------------------------------------------------------------------*/

/**
 * Creates an AST node representing a literal value.
 * @param token The Token representing the literal (number, string, bool, null).
 */
ASTNode *create_literal_node(Token token, Arena *a);

ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right, Arena *a);

ASTNode *create_var_decl_node(const char *name, int name_length,
                              DataType value_type, ASTNode *value, Fxn fxn,
                              int level, Arena *a, int pointer_level,
                              MemoryBucket bucket, int array_count);
/**
 * Creates an empty AST block container to hold statements.
 */
ASTNode *create_block_node(int level, Arena *a);

ASTNode *create_var_ref_node(const char *name, int name_length, Fxn fxn,
                             int level, Arena *a);

ASTNode *create_var_assign_node(const char *name, int name_length,
                                ASTNode *value, Fxn fxn, int level, Arena *a,
                                int deref_level);

ASTNode *create_if_node(ASTNode *condition, ASTNode *then_block,
                        ASTNode *else_block, Arena *a);

/**
 * Creates an AST node representing a while loop.
 * @param condition The AST node evaluating to the loop condition.
 * @param then_block The AST block to execute repeatedly while condition is
 * true.
 */
ASTNode *create_while_node(ASTNode *condition, ASTNode *then_block, Arena *a);

/**
 * Creates an AST node representing a for loop.
 * @param variable The AST node representing loop variable initialization.
 * @param condition The AST node representing loop continuation condition.
 * @param var_operation The AST node representing loop variable step/update.
 * @param then_block The AST block to execute repeatedly while condition is
 * true.
 */
ASTNode *create_for_node(ASTNode *variable, ASTNode *condition,
                         ASTNode *var_operation, ASTNode *then_block, Arena *a);

/**
 * Creates an AST node representing fxn call.
 * @param name The AST node representing the name of the fxn.
 * @param name_length The AST node representing the length of the name.
 * @param return_type The AST node representing the return type of the fxn.
 * @param fxn Pointer to the function symbol.
 * @param level The scope level of the function call.
 * @param args Pointer to the list of arguments passed to the function.
 */
ASTNode *create_fxn_call_node(char *name, int name_length, Fxn fxn, int level,
                              Args *args, Arena *a);

/**
 * Creates an AST node representing a function definition.
 * @param name The name of the function.
 * @param name_length Length of the function name.
 * @param body The AST block node representing the function body.
 */
ASTNode *create_function_node(const char *name, int name_length, ASTNode *body,
                              Fxn fxn, int level, Arena *a, int pointer_level);

/**
 * Creates an AST node representing the entire program.
 * @param block The block containing top-level declarations/functions.
 */
ASTNode *create_program_node(ASTNode *block, Arena *a);

/**
 * Creates an AST node representing a return statement.
 * @param context Pointer to the codegen context.
 * @param fxn Function scope containing the return statement.
 * @param value AST node representing the expression to return.
 * @param a Arena allocator for node allocation.
 * @return Pointer to the created AST return node.
 */
ASTNode *create_ret_node(CodegenContext *context, Fxn fxn, ASTNode *value,
                         Arena *a);

/**
 * Realloc more memory to the AST block
 * @param block The AST node representing the block to add.
 * @param statement The type of statement the block falls under.
 */
void block_add_statement(ASTNode *block, ASTNode *statement, Arena *a);

/**
 * Creates an AST node representing a unary operation.
 * @param operator_type The unary operator token type.
 * @param value AST node representing the target expression.
 * @param a Arena allocator for node allocation.
 * @return Pointer to the created AST unary node.
 */
ASTNode *create_urinary_node(TokenType operator_type, ASTNode *value, Arena *a);

ASTNode *create_array_node(ASTNode **elements, uint32_t count,
                           uint32_t capacity, bool is_dynamic,
                           DataType element_type, Arena *a);

ASTNode *create_index_expr_node(ASTNode *target, ASTNode *index,
                                DataType datatype, Arena *a);

ASTNode *create_struct_node(Struct *fields, Fxn *fxn, int level, Arena *a,
                            char *name, int name_length);

ASTNode *create_access_node(ASTNode *src, ASTNode *target, DataType dt,
                            Arena *a);

ASTNode *create_target_assign_node(ASTNode *target_node, ASTNode *value,
                                   Fxn fxn, int level, Arena *a);

ASTNode *create_fxns_node(char *name, int name_length, int level, Fxns *fxns,
                          Arena *a);

void fxns_add_method(ASTNode *node, ASTNode *method, Arena *a);

#endif
