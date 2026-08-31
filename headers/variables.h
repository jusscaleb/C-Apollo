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

#include <llvm-c/Core.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct Arena Arena;
typedef struct CodegenContext CodegenContext;

typedef struct CodegenContext CodegenContext;
// typedef struct  FxnCallMetaData FxnCallMetaData;
typedef struct Fxn Fxn;
typedef struct Struct Struct;
//typedef struct Struct Struct;

typedef struct FxnCallMetaData {
  LLVMTypeRef fxn_type;
  LLVMValueRef the_fxn;
}FxnCallMetaData;

typedef enum {
  TYPE_STRING,
  TYPE_INT,
  TYPE_CHAR,
  TYPE_FLOAT,
  TYPE_BOOL,
  TYPE_NULL,
  TYPE_INT_PTR,
  TYPE_FLOAT_PTR,
  TYPE_BOOL_PTR,
  TYPE_CHAR_PTR,
  TYPE_INT_ARRAY,
  TYPE_FLOAT_ARRAY,
  TYPE_BOOL_ARRAY,
  TYPE_STR_ARRAY,
  TYPE_CHAR_ARRAY,
  TYPE_STRUCT
} DataType;

typedef enum {
  BUCKET_PLUS_ONE,//Short lifespan 
  BUCKET_ONE, // Constants
  BUCKET_TWO, // Arena 
  BUCKET_THREE // Heap
}MemoryBucket;


extern const char *dt_names[];

// char *datatypes[] = {"str", "int", "char", "float", "bool", "null"};

typedef struct {
  DataType type; 
  const char *start; 
  int length;
  int line;
} Variable;

typedef enum {
  VAR,
  FUNC,
  FUNC_CALL,
  CLASS,
  OBJ,
  ARR,
} token_type;

typedef struct Symbol {
  int scope_level;
  int str_const_id;
  uint32_t name_length;
  DataType type;
  token_type t_type;
  char *name;
  char *llvm_name;
  Fxn *fxn;
  bool is_active;
  LLVMValueRef llvm_val_ref;
  FxnCallMetaData fxn_meta_data;
  int param_idx;
  int frame_index;
  int pointer_level;
  MemoryBucket bucket;
  int array_count;
  bool assigned;
  LLVMTypeRef llvm_struct_type;
  Struct *struct_fields;
  char *struct_type_name;
} Symbol;

typedef struct SymbolEntry {
  Symbol *sym;
  uint32_t hash;
  struct SymbolEntry *next;
} SymbolEntry;

typedef struct SymbolTable {

  SymbolEntry **buckets;
  uint32_t capacity;
  uint32_t count;

} SymbolTable;


/**
 * Registers a new variable in the symbol table.
 * @param context The codegen context containing the symbol table.
 * @param name The name of the variable.
 * @param variable_type The DataType of the variable.
 * @param fxn The function scope the variable belongs to.
 * @param level The nesting scope level of the variable.
 */
Symbol *register_variable(CodegenContext *context, const char *name,
                          DataType variable_type, Fxn *fxn, int level, int len);

/**
 * Looks up a token (variable or function) in the symbol table by searching
 * backwards.
 * @param context The codegen context.
 * @param name The identifier name to look up.
 * @param fxn The current function scope context.
 * @param level The current scope level context.
 * @return A pointer to the Symbol if found and active, otherwise NULL.
 */
Symbol *lookup_token(CodegenContext *context, const char *name, Fxn *fxn,
                     int level);

/**
 * Initializes a new SymbolTable using the provided arena allocator.
 * @param a Pointer to arena allocator.
 * @param initial_capacity Initial number of hash table buckets.
 * @return Pointer to initialized SymbolTable structure.
 */
SymbolTable *symbol_table_init(Arena *a, uint32_t initial_capacity);

/**
 * Resizes symbol table buckets array to accommodate growing symbol count.
 * @param table Pointer to SymbolTable structure.
 * @param a Pointer to arena allocator.
 */
void symbol_table_resize(SymbolTable *table, Arena *a);

/**
 * Inserts a Symbol entry into the hash table.
 * @param table Pointer to SymbolTable structure.
 * @param sym Pointer to Symbol to insert.
 * @param a Pointer to arena allocator.
 */
void symbol_table_insert(SymbolTable *table, Symbol *sym, Arena *a);

/**
 * Calculates FNV-1a hash value for a given character sequence.
 * @param key Character sequence to hash.
 * @param length Length of character sequence.
 * @return 32-bit hash value.
 */
uint32_t hash_string(const char *key, uint32_t length);

/**
 * Looks up an active symbol in the symbol table by string key.
 * @param table Pointer to SymbolTable structure.
 * @param name Symbol name string to query.
 * @param name_length Length of symbol name string.
 * @return Pointer to Symbol if found and active, NULL otherwise.
 */
Symbol *symbol_table_lookup(SymbolTable *table, const char *name, uint32_t name_length);

/**
 * Deactivates all symbols declared within a specified scope level.
 * @param table Pointer to SymbolTable structure.
 * @param scope_level Scope level exiting.
 */
void symbol_table_exit_scope(SymbolTable *table, int scope_level);

#endif
