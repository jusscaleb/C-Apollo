/*===================================================================
                          variables.h

                    (c)2026 SCXRPIUS.dev

              The Apollo Variables Compile Time library.
      Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/



#ifndef VARIABLES
#define VARIABLES


#include <stdio.h>
#include <string.h>
#include <stdbool.h>


typedef struct CodegenContext CodegenContext;
typedef struct Fxn Fxn ;


typedef enum {
  TYPE_STRING,
  TYPE_INT,
  TYPE_CHAR,
  TYPE_FLOAT,
  TYPE_BOOL,
  TYPE_NULL,
} DataType;

typedef struct {
  DataType type;     // Token type
  const char *start; // RAM Address of Token
  int length;
  int line;
} Variable;


typedef enum{
  VAR,
  FUNC,
  FUNC_CALL,
  CLASS,
  OBJ,
  ARR,

}token_type;

typedef struct Symbol {
  int scope_level;
  int str_length;
  DataType type;
  token_type t_type;
  char* name;
  char* llvm_name; 
  Fxn *fxn;
  bool is_active;
} Symbol;

/**
 * Registers a new variable in the symbol table.
 * @param context The codegen context containing the symbol table.
 * @param name The name of the variable.
 * @param variable_type The DataType of the variable.
 * @param fxn The function scope the variable belongs to.
 * @param level The nesting scope level of the variable.
 */
void register_variable(CodegenContext *context, const char *name,
                       DataType variable_type, Fxn *fxn, int level);

/**
 * Looks up a token (variable or function) in the symbol table by searching backwards.
 * @param context The codegen context.
 * @param name The identifier name to look up.
 * @param fxn The current function scope context.
 * @param level The current scope level context.
 * @return A pointer to the Symbol if found and active, otherwise NULL.
 */
Symbol *lookup_token(CodegenContext *context, const char *name, Fxn *fxn, int level);

/**
 * Creates an LLVM local variable (alloca) for a newly declared variable.
 * @param context The codegen context.
 * @param number_start The string pointer to the literal value or expression start.
 * @param length The length of the literal sequence.
 * @param name The LLVM identifier name for the variable.
 */
void create_var(CodegenContext *context, const char *number_start, int length,
                const char *name);

/**
 * Emits LLVM instructions to print a resolved variable to standard output.
 * @param context The codegen context.
 * @param sym The resolved Symbol pointer representing the variable.
 * @param scope_level The current scope level.
 */
void gen_println_variable(CodegenContext *context, Symbol *sym, int scope_level);

#endif
