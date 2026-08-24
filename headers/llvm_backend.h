/*===================================================================
                         llvm_backend.h

                    (c)2026 SCXRPIUS.dev

          LLVM API backend scaffold for Apollo code generation.
====================================================================*/

#ifndef APOLLO_LLVM_BACKEND_H
#define APOLLO_LLVM_BACKEND_H

#include "ast.h" 
#include "defs.h"
#include "token.h"
#include "variables.h"
#include "llvm-c/Analysis.h"
#include <llvm-c/Transforms/PassBuilder.h>

#include <llvm-c/Types.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Core.h>
#include <llvm-c/Linker.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Types.h>
#include <stdbool.h>
#include <stdint.h>

#define RUNTIME_LIBS                                                           \
  {"apollo-modules/apl-io/apl-io.bc",                                          \
   "apollo-modules/apl-string/apl-string.bc"}


#define VOID(x) (LLVMVoidTypeInContext(x))
#define I32(x)  (LLVMInt32TypeInContext(x))
#define F32(x)  (LLVMFloatTypeInContext(x))
#define I1(x)   (LLVMInt1TypeInContext(x))
#define I8(x)   (LLVMInt8TypeInContext(x))


typedef struct LLVMComponents {
  LLVMContextRef ctx;
  LLVMBuilderRef builder;
  LLVMModuleRef module;
  LLVMValueRef current_fxn;
  LLVMTypeRef current_frame_type;
  LLVMValueRef current_frame_alloc;
  LLVMValueRef current_parent_frame;
  Fxn *current_fxn_ast;
} LLVMComponents;


/**
 * Initializes the required LLVM environment, context, module, and targets.
 * @param components Structure holding LLVM context and state pointers.
 * @param program_node AST root program node to prepare generation for.
 */
void _apl_llvm_environment_setup(
                                 LLVMComponents *components,
                                 ASTNode *program_node);

/**
 * Shuts down LLVM context and disposes of module resources.
 * @param components Pointer to LLVMComponents state.
 */
void _apl_llvm_shutdown(LLVMComponents *components);

/**
 * Loads pre-compiled runtime bitcode libraries into the current LLVM module.
 * @param components Pointer to LLVMComponents state.
 */
void _apl_load_runtime_libraries(LLVMComponents *components);

/**
 * Imports built-in runtime functions from a bitcode file into the destination module.
 * @param dest_module Target LLVM module to link runtime declarations into.
 * @param filename File path to the bitcode module.
 */
void _apl_import_runtime(LLVMModuleRef dest_module, const char *filename);

/**
 * Saves generated LLVM IR bitcode/text to a file and shuts down LLVM resources.
 * @param components Pointer to LLVMComponents state.
 */
void _apl_save_and_shutdown(LLVMComponents *components);

/**
 * Generates LLVM IR code for an AST statement block.
 * @param components Pointer to LLVMComponents state.
 * @param block_node AST node representing the block container.
 */
void _apl_gen_block_from_ast(LLVMComponents *components,
                              ASTNode *block_node);

/**
 * Generates entry point sequence and frame setup for a function definition.
 * @param components Pointer to LLVMComponents state.
 * @param block_node AST node for the function definition.
 */
void _apl_gen_function_start(LLVMComponents *components,
                              ASTNode *block_node);

/**
 * Generates clean exit and ret instructions for function ending.
 * @param components Pointer to LLVMComponents state.
 * @param block_node AST node for the function definition.
 */
void _apl_gen_function_end(LLVMComponents *components, 
                           ASTNode *block_node);

/**
 * Generates alloca and local storage IR for a variable declaration node.
 * @param components Pointer to LLVMComponents state.
 * @param var_node AST node representing variable declaration.
 */
void _apl_create_local_variable(LLVMComponents *components,
                                 ASTNode *var_node);

/**
 * Resolves the DataType enum of a literal or identifier AST token node.
 * @param components Pointer to LLVMComponents state.
 * @param token_node AST node evaluating token datatype.
 * @return Resolved DataType of the expression node.
 */
DataType _apl_get_token_datatype(LLVMComponents *components,
                                  ASTNode *token_node);

/**
 * Generates IR instructions for a function invocation expression/statement.
 * @param components Pointer to LLVMComponents state.
 * @param stmt AST node representing the call expression.
 */
void _apl_gen_fxn_call_from_ast(LLVMComponents *components,
                                 ASTNode *stmt);

/**
 * Emits LLVM IR call to print runtime output functions.
 * @param components Pointer to LLVMComponents state.
 * @param args Linked list of argument expressions to print.
 */
void _apl_gen_println_ir(LLVMComponents *components, 
                         Args *args);

/**
 * Extracts raw character data from string token literal, unescaping characters into clean buffer.
 * @param string Token representing string literal.
 * @param clean_str Destination buffer for cleaned string text.
 */
void slice_string(Token string, char *clean_str);

/**
 * Parses single character token literal into a uint8_t byte value.
 * @param token Character literal token.
 * @return Unescaped byte value of character literal.
 */
uint8_t parse_char_literal(Token token);

/**
 * Emits LLVM load instruction for a variable reference AST node.
 * @param components Pointer to LLVMComponents state.
 * @param var_ref_node AST node representing variable reference.
 * @return LLVMValueRef representing loaded variable value.
 */
LLVMValueRef
load_variable(LLVMComponents *components, ASTNode *var_ref_node);

/**
 * Generates binary arithmetic or comparison LLVM instructions.
 * @param components Pointer to LLVMComponents state.
 * @param node AST node for binary expression.
 * @param result_name Temporary name tag for generated LLVM value.
 * @return LLVMValueRef result of arithmetic operation.
 */
LLVMValueRef arihmetics(LLVMComponents *components, ASTNode *node,
                        char *result_name);

/**
 * Generates store instruction to update variable value in memory.
 * @param components Pointer to LLVMComponents state.
 * @param node AST variable assignment node.
 */
void _apl_reassign_variable(LLVMComponents *components, 
                            ASTNode *node);

/**
 * Generates basic blocks and branching IR for if-elif-else construct.
 * @param components Pointer to LLVMComponents state.
 * @param node AST node representing if statement.
 */
void _apl_gen_if_block(LLVMComponents *components, 
                       ASTNode *node);

/**
 * Generates loop basic blocks and conditional jump IR for while loop node.
 * @param components Pointer to LLVMComponents state.
 * @param node AST node representing while loop.
 */
void _apl_gen_while_loop(LLVMComponents *components, 
                         ASTNode *node);

/**
 * Generates loop control structures, condition, and increment IR for for loop node.
 * @param components Pointer to LLVMComponents state.
 * @param node AST node representing for loop.
 */
void _apl_gen_for_loop(LLVMComponents *components, 
                       ASTNode *node);

/**
 * Converts up to k characters of string buffer into floating point number representation.
 * @param s Input string buffer.
 * @param k Number of characters to parse.
 * @return Evaluated float value.
 */
float str_to_int_k(const char *s, int k);

/**
 * Emits return IR instruction and return value expression for current function block.
 * @param components Pointer to LLVMComponents state.
 * @param node AST return statement node.
 */
void  _apl_gen_return(LLVMComponents *components,  ASTNode *node);

/**
 * Evaluates constant integer AST expression at compile time if applicable.
 * @param expr AST expression node.
 * @return Integer value result.
 */
float return_eval_int(ASTNode* expr);

/**
 * Evaluates function call node and returns generated LLVM call instruction value.
 * @param components Pointer to LLVMComponents state.
 * @param node AST function call node.
 * @return LLVMValueRef representing function invocation result.
 */
LLVMValueRef _apl_eval_function_call(LLVMComponents *components, ASTNode *node);

/**
 * Counts total number of parameters in a function parameter list.
 * @param components Pointer to LLVMComponents state.
 * @param p Pointer to head of parameter chain.
 * @return Number of parameters.
 */
uint32_t _apl_get_n_params(LLVMComponents *components, Params *p);

/**
 * Counts total number of arguments in a function argument list.
 * @param components Pointer to LLVMComponents state.
 * @param a Pointer to head of argument chain.
 * @return Number of arguments.
 */
uint32_t _apl_get_n_args(LLVMComponents *components, Args *a);

/**
 * Maps Apollo high-level DataType to corresponding LLVMTypeRef.
 * @param components Pointer to LLVMComponents state.
 * @param dt Apollo DataType enum value.
 * @return Corresponding LLVMTypeRef representation.
 */
LLVMTypeRef _apl_get_llvm_type(LLVMComponents*components, DataType dt);

/**
 * Generates IR instructions for reassigning string struct buffer pointers.
 * @param components Pointer to LLVMComponents state.
 * @param str_struct_ptr LLVM pointer to target string struct instance.
 * @param char_ptr Raw character sequence array.
 * @param struct_type LLVM struct type reference.
 */
void _apl_build_string_reassign(LLVMComponents *components, LLVMValueRef str_struct_ptr, char* char_ptr, LLVMTypeRef struct_type);

/**
 * Runs LLVM pass builder optimization passes on the module IR.
 * @param components Pointer to LLVMComponents state.
 */
void _apl_optimize_module(LLVMComponents *components);


#endif
