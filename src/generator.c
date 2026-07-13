/*--------------------------------------------------------------------------------

                         THE TRANSLATOR

---------------------------------------------------------------------------------*/

#include "../headers/arithmetic.h"
#include "../headers/ast.h"
#include "../headers/defs.h"
#include "../headers/token.h"
#include "../headers/variables.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Symbol *get_token(CodegenContext *context, const char *name);
static void gen_println_bool(CodegenContext *context, Token token);
static void create_var_from_expr_result(CodegenContext *context,
                                        ExprResult result, const char *name);

/*--------------------------------------------------------------------------------

                  INITIALIZING SEGMENT -> BEGIN

---------------------------------------------------------------------------------*/

// AST Dictionary
void gen_block_from_ast(CodegenContext *context, ASTNode *block_node) {
  if (block_node == NULL || block_node->Type != AST_BLOCK)
    return;

  for (int i = 0; i < block_node->block.count; i++) {
    ASTNode *stmt = block_node->block.statements[i];
    if (stmt == NULL)
      continue;

    switch (stmt->Type) {
    case AST_PRINTLN:
      gen_println_from_ast(context, stmt);
      break;
    case AST_VAR_DECL:
      gen_var_decl_from_ast(context, stmt);
      break;
    case AST_VAR_ASS:
      gen_var_assign_from_ast(context, stmt);
      break;
    case AST_IF:
      gen_if_from_ast(context, stmt);
      break;

    case AST_WHILE:
      gen_while_from_ast(context, stmt);
      break;
    case AST_FOR:
      gen_for_from_ast(context, stmt);
      break;
    case AST_CALL_FXN:
      gen_fxn_call_from_ast(context, stmt);
      break;
    case AST_FUNCTION: {
      if (stmt->function.fxn.parent_fxn != NULL && strcmp(stmt->function.fxn.parent_fxn->name, "global") != 0) {
        // It's a nested function, defer it to avoid nested LLVM definitions!
        if (context->deferred_count >= context->deferred_capacity) {
          context->deferred_capacity = context->deferred_capacity == 0 ? 8 : context->deferred_capacity * 2;
          context->deferred_functions = realloc(context->deferred_functions, context->deferred_capacity * sizeof(ASTNode*));
        }
        context->deferred_functions[context->deferred_count++] = stmt;
      } else {
        // It's a global function, generate normally
        gen_function_start(context, stmt);
        gen_block_from_ast(context, stmt->function.body);
        gen_function_end(context, strcmp(stmt->function.resolved_symbol->llvm_name, "run") == 0);
      }
      break;
    }

    default:
      fprintf(stderr, "Unsupported statement type in block codegen.\n");
      exit(EXIT_FAILURE);
    }
  }
}

void gen_program_from_ast(CodegenContext *context, ASTNode *program_node) {
  if (program_node == NULL || program_node->Type != AST_PROGRAM)
    return;
  
  // The program node holds a block of functions
  gen_block_from_ast(context, program_node->program.function);
  
  // Generate all nested functions at the top level
  for (int i = 0; i < context->deferred_count; i++) {
    ASTNode *nested_func = context->deferred_functions[i];
    gen_function_start(context, nested_func);
    gen_block_from_ast(context, nested_func->function.body);
    gen_function_end(context, false);
  }
}

// Opens the LLVM output file and resets the counters used for generated names.
void codegen_init(CodegenContext *context, const char *output_filename) {
  context->file = fopen(output_filename, "w");

  context->string_constant_count = 0;
  context->temp_count = 0;

  context->symbol_count = 0;
  context->symbol_capacity = 0;
  context->symbols = NULL;
  
  context->deferred_functions = NULL;
  context->deferred_count = 0;
  context->deferred_capacity = 0;

  if (context->file == NULL) {
    fprintf(stderr, "Could not create output file %s\n", output_filename);
    exit(EXIT_FAILURE);
  }

  fprintf(context->file, "; --- Apollo Native Compiler Backend Output ---\n");

  fprintf(context->file, "declare i8* @printf(i8*, ...)\n\n");
  fprintf(context->file, "declare i8* @malloc(i64)\n");
  fprintf(context->file, "declare i8* @strcat(i8*, i8*)\n");
  fprintf(context->file, "declare i8* @strcpy(i8*, i8*)\n");
  fprintf(context->file, "declare i32 @strcmp(i8*, i8*)\n");
  fprintf(context->file, "declare i32 @snprintf(i8*, i64, i8*, ...)\n");
}

static const char *llvm_datatype(DataType type);

// Emits the LLVM function header. Apollo's run() function becomes LLVM's
// main().
void gen_function_start(CodegenContext *context, ASTNode *func_node) {
  const char *name = func_node->function.resolved_symbol->llvm_name;
  // if it run then make that the main function.
  if (strcmp(name, "run") == 0) {
    fprintf(context->file, "define i32 @main() {\n");
  } else {
    fprintf(context->file, "define void @%s(", name);
    
    // Print the parameters in the signature: e.g. i32 %arg_x, i8* %arg_s
    Params *p = func_node->function.fxn.params;
    while(p) {
      DataType dt = p->param->var_decl.value_type;
      const char *type_str = llvm_datatype(dt);
      int nl = p->param->var_decl.name_length;
      const char *pname = p->param->var_decl.name;
      fprintf(context->file, "%s %%arg_%.*s", type_str, nl, pname);
      if (p->next) fprintf(context->file, ", ");
      p = p->next;
    }
    
    fprintf(context->file, ") {\n");
    
    // Alloca and store each parameter so the body can reference them
    p = func_node->function.fxn.params;
    while(p) {
      DataType dt = p->param->var_decl.value_type;
      const char *type_str = llvm_datatype(dt);
      int nl = p->param->var_decl.name_length;
      const char *pname = p->param->var_decl.name;

      if (dt == TYPE_STRING) {
        // Strings: create the _val (i8**) and _len (i32) slots that VAR_REF expects
        fprintf(context->file, "  %%%.*s_val = alloca i8*\n", nl, pname);
        fprintf(context->file, "  %%%.*s_len = alloca i32\n", nl, pname);
        fprintf(context->file, "  store i8* %%arg_%.*s, i8** %%%.*s_val\n",
                nl, pname, nl, pname);
      } else {
        // Scalar types (int, float, bool, char)
        fprintf(context->file, "  %%%.*s = alloca %s\n", nl, pname, type_str);
        fprintf(context->file, "  store %s %%arg_%.*s, %s* %%%.*s\n",
                type_str, nl, pname, type_str, nl, pname);
      }
      p = p->next;
    }
  }
}

/*--------------------------------------------------------------------------------

                  INITIALIZING SEGMENT -> END

---------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------

                  OUTPUT SEGMENT -> BEGIN

---------------------------------------------------------------------------------*/

// Counts how many runtime bytes a string literal needs after escape sequences
// like \n, \t, \", and \\ are converted into one actual character.
static int escaped_string_byte_count(const char *string_start, int length) {
  int count = 0;

  for (int i = 0; i < length; i++) {
    if (string_start[i] == '\\' && i + 1 < length) {
      i++;
    }

    count++;
  }

  return count;
}

// Writes string contents in LLVM's escaped format.
// Example: a newline in Apollo becomes \0A in the generated LLVM string.
static void emit_llvm_string_contents(FILE *file, const char *string_start,
                                      int length) {
  for (int i = 0; i < length; i++) {
    if (string_start[i] == '\\' && i + 1 < length) {
      i++;

      switch (string_start[i]) {
      case 'n':
        fprintf(file, "\\0A");
        break;
      case 't':
        fprintf(file, "\\09");
        break;
      case '"':
        fprintf(file, "\\22");
        break;
      case '\\':
        fprintf(file, "\\5C");
        break;
      default:
        fprintf(file, "%c", string_start[i]);
        break;
      }

      continue;
    }

    switch (string_start[i]) {
    case '"':
      fprintf(file, "\\22");
      break;
    case '\\':
      fprintf(file, "\\5C");
      break;
    default:
      fprintf(file, "%c", string_start[i]);
      break;
    }
  }
}

// Emits LLVM that prints an Apollo string literal with printf.
void gen_println_statement(CodegenContext *context, const char *string_start,
                           int length) {
  int id = context->string_constant_count++;
  int internal_len =
      length - 1; // removing the quotes from the high level syntax.
  int runtime_len = escaped_string_byte_count(string_start + 1, internal_len);
  int llvm_len = runtime_len + 2; // include newline and null terminator.
  fprintf(context->file, "; Allocate string literal block in memory\n");
  // Copy the Apollo string literal into a local LLVM string buffer.
  fprintf(context->file, "%%str_%d = alloca [%d x i8]\n", id, llvm_len);
  fprintf(context->file, "store [%d x i8] c\"", llvm_len);
  emit_llvm_string_contents(context->file, string_start + 1, internal_len);
  fprintf(context->file, "\\0A\\00\", [%d x i8]* %%str_%d\n", llvm_len, id);
  fprintf(context->file,
          "%%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* "
          "%%str_%d, i32 0, i32 0\n",
          id, llvm_len, llvm_len, id);

  // Call the printf operation
  fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%str_ptr_%d)\n\n",
          id);
}

static void gen_println_bool(CodegenContext *context, Token token) {
  int id = context->string_constant_count++;
  int llvm_len = token.length + 2;

  fprintf(context->file, "; Allocate bool literal block in memory\n");
  fprintf(context->file, "%%bool_%d = alloca [%d x i8]\n", id, llvm_len);
  fprintf(context->file,
          "store [%d x i8] c\"%.*s\\0A\\00\", [%d x i8]* %%bool_%d\n", llvm_len,
          token.length, token.start, llvm_len, id);
  fprintf(context->file,
          "%%bool_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* "
          "%%bool_%d, i32 0, i32 0\n",
          id, llvm_len, llvm_len, id);
  fprintf(context->file, "call i32 (i8*, ...) @printf(i8* %%bool_ptr_%d)\n\n",
          id);
}

void gen_println_from_ast(CodegenContext *context, ASTNode *println_node) {
  ASTNode *value = println_node->println.value;

  if (value->Type == AST_LITERAL_EXPR) {
    Token token = value->literal_expr.token;

    switch (token.type) {
    case TOKEN_STRING:
      gen_println_statement(context, token.start, token.length);
      break;

    case TOKEN_INT:
      gen_println_integer(context, token.start, token.length);
      break;

    case TOKEN_FLOAT:
      gen_println_float(context, token.start, token.length);
      break;

    case TOKEN_BOOL:
    case TOKEN_NULL:
      gen_println_bool(context, token);
      break;

    default:
      fprintf(stderr, "Unsupported literal in println AST.\n");
      exit(EXIT_FAILURE);
    }

    return;
  }

  if (value->Type == AST_BINARY_EXPR) {
    ExprResult result = gen_expr_from_ast(context, value);
    gen_println_expr(context, result);
    return;
  }
  if (value->Type == AST_VAR_REF) {
    gen_println_variable(context, (Symbol*)value->var_ref.resolved_symbol, 0);
    return;
  }

  fprintf(stderr, "Unsupported value in println AST.\n");
  exit(EXIT_FAILURE);
}

// Emits LLVM that prints a single integer literal.
void gen_println_integer(CodegenContext *context, const char *number_start,
                         int length) {
  int id = context->string_constant_count++;

  fprintf(context->file, "; Allocate integer printf format block in memory\n");
  fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
  fprintf(context->file,
          "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
  fprintf(context->file,
          "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
          "%%int_fmt_%d, i32 0, i32 0\n",
          id, id);
  fprintf(context->file,
          "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %.*s)\n\n", id,
          length, number_start);
}

// Emits LLVM that prints a single decimal literal as a double.
void gen_println_float(CodegenContext *context, const char *float_start,
                       int length) {
  int id = context->string_constant_count++;

  fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
  fprintf(context->file,
          "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
  fprintf(context->file,
          "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
          "%%float_fmt_%d, i32 0, i32 0\n",
          id, id);
  fprintf(
      context->file,
      "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %.*s)\n\n",
      id, length, float_start);
}

/*--------------------------------------------------------------------------------

                  OUTPUT SEGMENT -> END

---------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------

                  VARIABLES SEGMENT -> BEGIN

---------------------------------------------------------------------------------*/

static const char *llvm_datatype(DataType type) {
  switch (type) {
  case TYPE_INT:
    return "i32";

  case TYPE_FLOAT:
    return "double";

  case TYPE_BOOL:
    return "i8";

  case TYPE_STRING:
    return "i8*";

  case TYPE_CHAR:
    return "i8";

  case TYPE_NULL:
    return "i8";

  default:
    return "i32"; // safe fallback
  }
}

// Returns the LLVM type string for a computed ExprResult
static const char *llvm_type_for_expr(ExprResult r) {
  switch (r.type) {
  case EXPR_FLOAT:  return "double";
  case EXPR_STRING: return "i8*";
  case EXPR_BOOL:   return "i8";
  default:          return "i32";
  }
}

static void create_string_var(CodegenContext *context, const char *value_start,
                              int length, const char *llvm_name) {
  int id = context->string_constant_count++;
  int internal_len = length - 1;   // removing the quotes
  int llvm_len = internal_len + 1; // include null terminator

  // 1. Allocate local pointers slots
  fprintf(context->file, "%%%s_val = alloca i8*\n", llvm_name);
  fprintf(context->file, "%%%s_len = alloca i32\n", llvm_name);

  // 2. Allocate the local stack string buffer
  fprintf(context->file, "  %%str_loc_%d = alloca [%d x i8]\n", id, llvm_len);
  fprintf(context->file, "  store [%d x i8] c\"", llvm_len);
  emit_llvm_string_contents(context->file, value_start + 1, internal_len);
  fprintf(context->file, "\\00\", [%d x i8]* %%str_loc_%d\n", llvm_len, id);

  // 3. Get pointer to the stack buffer and store it in variable slots
  fprintf(context->file,
          "  %%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* "
          "%%str_loc_%d, i32 0, i32 0\n",
          id, llvm_len, llvm_len, id);
  fprintf(context->file, "  store i8* %%str_ptr_%d, i8** %%%s_val\n", id,
          llvm_name);
  fprintf(context->file, "  store i32 %d, i32* %%%s_len\n\n", internal_len,
          llvm_name);
}

// Emits a GLOBAL string variable at the top level using LLVM @global syntax.
static void create_global_string_var(CodegenContext *context,
                                     const char *value_start, int length,
                                     const char *llvm_name) {
  int id = context->string_constant_count++;
  int internal_len = length - 1;   // strip quotes
  int llvm_len = internal_len + 1; // include null terminator

  // Emit a constant string array at global scope
  fprintf(context->file, "@%s_str_%d = private unnamed_addr constant [%d x i8] c\"",
          llvm_name, id, llvm_len);
  emit_llvm_string_contents(context->file, value_start + 1, internal_len);
  fprintf(context->file, "\\00\"\n");

  // Emit a global pointer that holds the address of the string
  fprintf(context->file, "@%s_val = global i8* getelementptr inbounds "
          "([%d x i8], [%d x i8]* @%s_str_%d, i32 0, i32 0)\n",
          llvm_name, llvm_len, llvm_len, llvm_name, id);
  fprintf(context->file, "@%s_len = global i32 %d\n\n", llvm_name, internal_len);
}

// Emits a GLOBAL numeric/bool variable at top level using LLVM @global syntax.
static void create_global_var(CodegenContext *context,
                              const char *number_start, int length,
                              const char *name) {
  Symbol *sym = get_token(context, name);
  char *llvm_name = sym->llvm_name;

  if (sym->type == TYPE_STRING) {
    if (length == 4 && strncmp(number_start, "null", 4) == 0) {
      create_global_string_var(context, "\"\"", 1, llvm_name);
    } else {
      create_global_string_var(context, number_start, length, llvm_name);
    }
  } else {
    const char *llvm_type = llvm_datatype(sym->type);
    if (sym->type == TYPE_BOOL || sym->type == TYPE_NULL) {
      int val = 2;
      if (length == 4 && strncmp(number_start, "true", 4) == 0)
        val = 1;
      else if (length == 5 && strncmp(number_start, "false", 5) == 0)
        val = 0;
      fprintf(context->file, "@%s = global i8 %d\n\n", llvm_name, val);
    } else {
      if (length == 4 && strncmp(number_start, "null", 4) == 0) {
        if (sym->type == TYPE_FLOAT) {
          fprintf(context->file, "@%s = global %s 0.0\n\n", llvm_name, llvm_type);
        } else {
          fprintf(context->file, "@%s = global %s 0\n\n", llvm_name, llvm_type);
        }
      } else {
        fprintf(context->file, "@%s = global %s %.*s\n\n",
                llvm_name, llvm_type, length, number_start);
      }
    }
  }
}

// Emits LLVM for a literal variable declaration.
void create_var(CodegenContext *context, const char *number_start, int length,
                const char *name) {

  Symbol *sym = get_token(context, name);
  char *llvm_name = sym->llvm_name;

  fprintf(context->file, "; Allocate integer variable slot\n");

  if (sym->type == TYPE_STRING) {
    if (length == 4 && strncmp(number_start, "null", 4) == 0) {
      create_string_var(context, "\"\"", 1, llvm_name);
    } else {
      create_string_var(context, number_start, length, llvm_name);
    }

  } else if (sym->type == TYPE_BOOL || sym->type == TYPE_NULL) {
    fprintf(context->file, "%%%s = alloca i8\n", llvm_name);
    int val = 2; // Default to null
    if (length == 4 && strncmp(number_start, "true", 4) == 0)
      val = 1;
    else if (length == 5 && strncmp(number_start, "false", 5) == 0)
      val = 0;
    fprintf(context->file, "store i8 %d, i8* %%%s\n\n", val, llvm_name);

  } else {
    const char *llvm_type = llvm_datatype(sym->type);
    fprintf(context->file, "%%%s = alloca %s\n", llvm_name, llvm_type);
    if (length == 4 && strncmp(number_start, "null", 4) == 0) {
      if (sym->type == TYPE_FLOAT) {
        fprintf(context->file, "store %s 0.0, %s* %%%s\n\n", llvm_type, llvm_type, llvm_name);
      } else {
        fprintf(context->file, "store %s 0, %s* %%%s\n\n", llvm_type, llvm_type, llvm_name);
      }
    } else {
      fprintf(context->file, "store %s %.*s, %s* %%%s\n\n", llvm_type, length,
              number_start, llvm_type, llvm_name);
    }
  }
}

static void create_var_from_expr_result(CodegenContext *context,
                                        ExprResult result, const char *name) {
  Symbol *sym = get_token(context, name);
  char *llvm_name = sym->llvm_name;

  if (result.type == EXPR_FLOAT) {
    fprintf(context->file, "; Allocate decimal expression variable slot\n");
    fprintf(context->file, "%%%s = alloca double\n", llvm_name);
    fprintf(context->file, "store double %s, double* %%%s\n\n", result.value,
            llvm_name);
    return;
  }

  if (result.type == EXPR_BOOL) {
    fprintf(context->file, "; Allocate bool expression variable slot\n");
    fprintf(context->file, "%%%s = alloca i8\n", llvm_name);
    // Truncate i32 (0 or 1) down to i8 for storage
    int trunc_id = context->temp_count++;
    fprintf(context->file, "%%tmp_%d = trunc i32 %s to i8\n", trunc_id,
            result.value);
    fprintf(context->file, "store i8 %%tmp_%d, i8* %%%s\n\n", trunc_id,
            llvm_name);
    return;
  }

  fprintf(context->file, "; Allocate integer expression variable slot\n");
  fprintf(context->file, "%%%s = alloca i32\n", llvm_name);
  fprintf(context->file, "store i32 %s, i32* %%%s\n\n", result.value,
          llvm_name);
}

void gen_var_decl_from_ast(CodegenContext *context, ASTNode *var_node) {

  if (var_node->Type != AST_VAR_DECL) {
    fprintf(stderr, "Expected variable declaration AST node.\n");
    exit(EXIT_FAILURE);
  }

  ASTNode *value = var_node->var_decl.value;
  const int NAME_LENGTH = var_node->var_decl.name_length;
  int level = var_node->var_decl.level;
  char name_buf[NAME_LENGTH + 1];
  snprintf(name_buf, sizeof(name_buf), "%.*s", NAME_LENGTH,
           var_node->var_decl.name);

  // Global variable — emit at top level with @name = global syntax
  if (level == 0) {
    if (value->Type == AST_LITERAL_EXPR) {
      Token token = value->literal_expr.token;
      create_global_var(context, token.start, token.length, name_buf);
      return;
    }
    fprintf(stderr, "Global variables only support literal initializers.\n");
    exit(EXIT_FAILURE);
  }

  // Local variable — emit alloca inside function body
  if (value->Type == AST_LITERAL_EXPR) {
    Token token = value->literal_expr.token;
    create_var(context, token.start, token.length, name_buf);
    return;
  }

  if (value->Type == AST_BINARY_EXPR || value->Type == AST_VAR_REF) {
    ExprResult result = gen_expr_from_ast(context, value);
    create_var_from_expr_result(context, result, name_buf);
    return;
  }

  fprintf(
      stderr,
      "Expected literal or arithmetic expression in variable declaration.\n");
  exit(EXIT_FAILURE);
}

void gen_var_assign_from_ast(CodegenContext *context, ASTNode *assign_node) {
  Symbol *sym = (Symbol*)assign_node->var_assign.resolved_symbol;
  if (!sym) {
    fprintf(stderr, "Error: Variable not resolved.\n");
    exit(EXIT_FAILURE);
  }

  ASTNode *value = assign_node->var_assign.value;

  int is_global = (sym->scope_level == 0 && (!sym->fxn || strcmp(sym->fxn->name, "global") == 0));
  // Macro-like helpers to pick @ vs % prefix
  #define VAL_PTR(buf, lname)  \
    snprintf(buf, sizeof(buf), "%s%s_val", is_global ? "@" : "%", lname)
  #define LEN_PTR(buf, lname)  \
    snprintf(buf, sizeof(buf), "%s%s_len", is_global ? "@" : "%", lname)
  #define SYM_PTR(buf, lname)  \
    snprintf(buf, sizeof(buf), "%s%s",     is_global ? "@" : "%", lname)

  if (value->Type == AST_VAR_REF) {
    Symbol *rhs_sym = (Symbol*)value->var_ref.resolved_symbol;
    int rhs_global = rhs_sym && (rhs_sym->scope_level == 0 && (!rhs_sym->fxn || strcmp(rhs_sym->fxn->name, "global") == 0));

    if (rhs_sym) {
      if (rhs_sym->type == TYPE_STRING) {
        int temp_ptr = context->temp_count++;
        int temp_len = context->temp_count++;
        char dst_val[80], dst_len[80];
        VAL_PTR(dst_val, sym->llvm_name);
        LEN_PTR(dst_len, sym->llvm_name);
        fprintf(context->file, "  %%tmp_%d = load i8*, i8** %s%s_val\n",
                temp_ptr, rhs_global ? "@" : "%", rhs_sym->llvm_name);
        fprintf(context->file, "  %%tmp_%d = load i32, i32* %s%s_len\n",
                temp_len, rhs_global ? "@" : "%", rhs_sym->llvm_name);
        fprintf(context->file, "  store i8* %%tmp_%d, i8** %s\n", temp_ptr, dst_val);
        fprintf(context->file, "  store i32 %%tmp_%d, i32* %s\n\n", temp_len, dst_len);
        return;
      }
      if (rhs_sym->type == TYPE_BOOL) {
        int temp_bool = context->temp_count++;
        char dst[80];
        SYM_PTR(dst, sym->llvm_name);
        fprintf(context->file, "  %%tmp_%d = load i8, i8* %s%s\n",
                temp_bool, rhs_global ? "@" : "%", rhs_sym->llvm_name);
        fprintf(context->file, "  store i8 %%tmp_%d, i8* %s\n\n", temp_bool, dst);
        return;
      }
    }
  }

  if (value->Type == AST_LITERAL_EXPR) {
    Token token = value->literal_expr.token;

    if (token.type == TOKEN_STRING) {
      int id = context->string_constant_count++;
      int internal_len = token.length - 1;
      int llvm_len = internal_len + 1;

      // Null upgrade — only valid for locals
      if (sym->type == TYPE_NULL && !is_global) {
        fprintf(context->file, "  %%%s_val = alloca i8*\n", sym->llvm_name);
        fprintf(context->file, "  %%%s_len = alloca i32\n", sym->llvm_name);
        sym->type = TYPE_STRING;
      }

      char dst_val[80], dst_len[80];
      VAL_PTR(dst_val, sym->llvm_name);
      LEN_PTR(dst_len, sym->llvm_name);

      fprintf(context->file, "  %%str_loc_%d = alloca [%d x i8]\n", id, llvm_len);
      fprintf(context->file, "  store [%d x i8] c\"", llvm_len);
      emit_llvm_string_contents(context->file, token.start + 1, internal_len);
      fprintf(context->file, "\\00\", [%d x i8]* %%str_loc_%d\n", llvm_len, id);

      fprintf(context->file,
              "  %%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* "
              "%%str_loc_%d, i32 0, i32 0\n",
              id, llvm_len, llvm_len, id);

      fprintf(context->file, "  store i8* %%str_ptr_%d, i8** %s\n", id, dst_val);
      fprintf(context->file, "  store i32 %d, i32* %s\n\n", internal_len, dst_len);
      return;

    } else if (token.type == TOKEN_BOOL || token.type == TOKEN_NULL) {
      int val = 2;
      if (token.type == TOKEN_BOOL) {
        val = (strncmp(token.start, "true", token.length) == 0) ? 1 : 0;
      }
      char dst[80]; SYM_PTR(dst, sym->llvm_name);
      fprintf(context->file, "  store i8 %d, i8* %s\n\n", val, dst);
      return;
    }
  }

  ExprResult result = gen_expr_from_ast(context, value);

  if (result.type == EXPR_FLOAT) {
    if (sym->type == TYPE_NULL && !is_global) {
      char new_name[72];
      snprintf(new_name, sizeof(new_name), "%s_fslot", sym->llvm_name);
      fprintf(context->file, "  %%%s = alloca double\n", new_name);
      snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", new_name);
      sym->type = TYPE_FLOAT;
    }
    char dst[80]; SYM_PTR(dst, sym->llvm_name);
    fprintf(context->file, "  store double %s, double* %s\n\n", result.value, dst);

  } else if (result.type == EXPR_INT) {
    if (sym->type == TYPE_BOOL) {
      char dst[80]; SYM_PTR(dst, sym->llvm_name);
      fprintf(context->file, "  store i8 %s, i8* %s\n", result.value, dst);
    } else if (sym->type == TYPE_NULL && !is_global) {
      char new_name[72];
      snprintf(new_name, sizeof(new_name), "%s_islot", sym->llvm_name);
      fprintf(context->file, "  %%%s = alloca i32\n", new_name);
      snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", new_name);
      sym->type = TYPE_INT;
      char dst[80]; SYM_PTR(dst, sym->llvm_name);
      fprintf(context->file, "  store i32 %s, i32* %s\n\n", result.value, dst);
    } else {
      char dst[80]; SYM_PTR(dst, sym->llvm_name);
      fprintf(context->file, "  store i32 %s, i32* %s\n", result.value, dst);
    }
  } else if (result.type == EXPR_BOOL) {
    if (sym->type == TYPE_NULL || sym->type == TYPE_BOOL) {
      int trunc_id = context->temp_count++;
      char dst[80]; SYM_PTR(dst, sym->llvm_name);
      fprintf(context->file, "  %%tmp_%d = trunc i32 %s to i8\n", trunc_id, result.value);
      fprintf(context->file, "  store i8 %%tmp_%d, i8* %s\n\n", trunc_id, dst);
      sym->type = TYPE_BOOL;
    }
  }

  #undef VAL_PTR
  #undef LEN_PTR
  #undef SYM_PTR
}



/*--------------------------------------------------------------------------------

                  VARIABLES SEGMENT -> END

---------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------

                  ARITHMETICS SEGMENT -> BEGIN

---------------------------------------------------------------------------------*/

// Converts a lexer token into an expression result.
// The value is kept as text so it can be written directly into LLVM later.

ExprResult gen_expr_from_ast(CodegenContext *context, ASTNode *expr) {
  switch (expr->Type) {
  case AST_LITERAL_EXPR: {
    Token token = expr->literal_expr.token;
    if (token.type == TOKEN_STRING) {
      int id = context->string_constant_count++;
      int internal_len = token.length - 1; // removing the quotes
      int llvm_len = internal_len + 1;     // include null terminator

      // 1. Allocate the local stack string buffer
      fprintf(context->file, "  %%str_expr_%d = alloca [%d x i8]\n", id,
              llvm_len);
      fprintf(context->file, "  store [%d x i8] c\"", llvm_len);
      emit_llvm_string_contents(context->file, token.start + 1, internal_len);
      fprintf(context->file, "\\00\", [%d x i8]* %%str_expr_%d\n", llvm_len,
              id);

      // 2. Get its pointer
      int temp_id = context->temp_count++;
      fprintf(context->file,
              "  %%str_expr_ptr_%d = getelementptr inbounds [%d x i8], [%d x "
              "i8]* %%str_expr_%d, i32 0, i32 0\n",
              temp_id, llvm_len, llvm_len, id);

      ExprResult result;
      result.type = EXPR_STRING;
      result.str_len = internal_len;
      snprintf(result.value, sizeof(result.value), "%%str_expr_ptr_%d",
               temp_id);
      return result;
    }
    return make_literal_expr(token);
  }

  case AST_BINARY_EXPR: {
    ExprResult left = gen_expr_from_ast(context, expr->binary_expr.left);
    ExprResult right = gen_expr_from_ast(context, expr->binary_expr.right);

    return gen_binary_expr(context, left, expr->binary_expr.operator_type,
                           right);
  }
  case AST_VAR_REF: {
    Symbol *sym = (Symbol*)expr->var_ref.resolved_symbol;
    
    int temp_id = context->temp_count++;

    if (sym->type == TYPE_STRING) {
      fprintf(context->file, "  %%tmp_%d = load i8*, i8** %%%s_val\n", temp_id,
              sym->llvm_name);
      ExprResult result;
      result.type = EXPR_STRING;
      result.str_len = sym->str_length; // Use the symbol's tracked length
      snprintf(result.value, sizeof(result.value), "%%tmp_%d", temp_id);
      return result;
    }

    const char *llvm_type = llvm_datatype(sym->type);
    fprintf(context->file, "%%tmp_%d = load %s, %s* %%%s\n", temp_id, llvm_type,
            llvm_type, sym->llvm_name);
    ExprResult result;
    result.type = (sym->type == TYPE_FLOAT) ? EXPR_FLOAT : EXPR_INT;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", temp_id);
    return result;
  }

  case AST_CALL_FXN: {
    const char *func_name = expr->call_fxn.resolved_symbol->llvm_name;

    // 1. Evaluate all argument expressions first, collect results
    Args *arg = expr->call_fxn.args;
    // We need to collect results before building the call string
    // Use a small fixed buffer (max 32 args should be plenty)
    ExprResult arg_results[32];
    DataType   arg_types[32];
    int arg_count = 0;
    Params *param = NULL;
    if (expr->call_fxn.resolved_symbol && expr->call_fxn.resolved_symbol->fxn) {
      param = expr->call_fxn.resolved_symbol->fxn->params;
    }
    while (arg && arg_count < 32) {
      arg_results[arg_count] = gen_expr_from_ast(context, arg->arg);
      arg_types[arg_count] = (param) ? param->param->var_decl.value_type : TYPE_INT;
      arg_count++;
      arg  = arg->next;
      if (param) param = param->next;
    }

    const char *llvm_type = llvm_datatype(expr->call_fxn.return_type);
    if (llvm_type == NULL) {
      llvm_type = "void";
    }
    int temp_id = context->temp_count++;
    if (expr->call_fxn.return_type == TYPE_NULL) {
      fprintf(context->file, "  call void @%s(", func_name);
    } else {
      fprintf(context->file, "  %%tmp_%d = call %s @%s(", temp_id, llvm_type, func_name);
    }
    // 2. Emit argument list
    for (int i = 0; i < arg_count; i++) {
      const char *atype = llvm_datatype(arg_types[i]);
      if (atype == NULL) atype = "i32";
      fprintf(context->file, "%s %s", atype, arg_results[i].value);
      if (i < arg_count - 1) fprintf(context->file, ", ");
    }
    fprintf(context->file, ")\n");

    if (expr->call_fxn.return_type == TYPE_NULL) {
      ExprResult result;
      result.type = EXPR_INT;
      snprintf(result.value, sizeof(result.value), "0");
      return result;
    } else {
      ExprResult result;
      result.type = (expr->call_fxn.return_type == TYPE_FLOAT) ? EXPR_FLOAT : EXPR_INT;
      snprintf(result.value, sizeof(result.value), "%%tmp_%d", temp_id);
      return result;
    }
  }

  default:
    fprintf(stderr, "Unsupported expression AST node.\n");
    exit(EXIT_FAILURE);
  }
}

ExprResult make_literal_expr(Token token) {
  ExprResult result;

  if (token.type == TOKEN_FLOAT) {
    result.type = EXPR_FLOAT;
    snprintf(result.value, sizeof(result.value), "%.*s", token.length,
             token.start);
  } else if (token.type == TOKEN_BOOL) {
    result.type = EXPR_BOOL;
    if (token.length == 4 && strncmp(token.start, "true", 4) == 0) {
      snprintf(result.value, sizeof(result.value), "1");
    } else {
      snprintf(result.value, sizeof(result.value), "0");
    }
  } else {
    result.type = EXPR_INT;
    snprintf(result.value, sizeof(result.value), "%.*s", token.length,
             token.start);
  }

  return result;
}

static ExprResult convert_to_string_expr(CodegenContext *context, ExprResult expr) {
  if (expr.type == EXPR_STRING) return expr;

  int fmt_id = context->string_constant_count++;
  int buf_id = context->temp_count++;
  int call_id = context->temp_count++;

  if (expr.type == EXPR_INT) {
    fprintf(context->file, "  %%fmt_%d = alloca [3 x i8]\n", fmt_id);
    fprintf(context->file, "  store [3 x i8] c\"%%d\\00\", [3 x i8]* %%fmt_%d\n", fmt_id);
    fprintf(context->file, "  %%fmt_ptr_%d = getelementptr inbounds [3 x i8], [3 x i8]* %%fmt_%d, i32 0, i32 0\n", fmt_id, fmt_id);
    fprintf(context->file, "  %%tmp_%d = call i8* @malloc(i64 16)\n", buf_id);
    fprintf(context->file, "  %%tmp_%d = call i32 (i8*, i64, i8*, ...) @snprintf(i8* %%tmp_%d, i64 16, i8* %%fmt_ptr_%d, i32 %s)\n", call_id, buf_id, fmt_id, expr.value);
    expr.str_len = 15;
  } else if (expr.type == EXPR_FLOAT) {
    fprintf(context->file, "  %%fmt_%d = alloca [3 x i8]\n", fmt_id);
    fprintf(context->file, "  store [3 x i8] c\"%%f\\00\", [3 x i8]* %%fmt_%d\n", fmt_id);
    fprintf(context->file, "  %%fmt_ptr_%d = getelementptr inbounds [3 x i8], [3 x i8]* %%fmt_%d, i32 0, i32 0\n", fmt_id, fmt_id);
    fprintf(context->file, "  %%tmp_%d = call i8* @malloc(i64 32)\n", buf_id);
    fprintf(context->file, "  %%tmp_%d = call i32 (i8*, i64, i8*, ...) @snprintf(i8* %%tmp_%d, i64 32, i8* %%fmt_ptr_%d, double %s)\n", call_id, buf_id, fmt_id, expr.value);
    expr.str_len = 31;
  } else if (expr.type == EXPR_BOOL) {
    fprintf(context->file, "  %%fmt_%d = alloca [3 x i8]\n", fmt_id);
    fprintf(context->file, "  store [3 x i8] c\"%%d\\00\", [3 x i8]* %%fmt_%d\n", fmt_id);
    fprintf(context->file, "  %%fmt_ptr_%d = getelementptr inbounds [3 x i8], [3 x i8]* %%fmt_%d, i32 0, i32 0\n", fmt_id, fmt_id);
    fprintf(context->file, "  %%tmp_%d = call i8* @malloc(i64 16)\n", buf_id);
    fprintf(context->file, "  %%tmp_%d = call i32 (i8*, i64, i8*, ...) @snprintf(i8* %%tmp_%d, i64 16, i8* %%fmt_ptr_%d, i32 %s)\n", call_id, buf_id, fmt_id, expr.value);
    expr.str_len = 15;
  }

  expr.type = EXPR_STRING;
  snprintf(expr.value, sizeof(expr.value), "%%tmp_%d", buf_id);
  return expr;
}

// Emits LLVM for a binary arithmetic expression such as left + right.
// Returns a new ExprResult pointing at the generated temporary value.
ExprResult gen_binary_expr(CodegenContext *context, ExprResult left,
                           TokenType operator_type, ExprResult right) {
  ExprResult result;

  // == and != on mismatched types always return false/true respectively
  if ((operator_type == TOKEN_EQT || operator_type == TOKEN_NEQ) &&
      left.type != right.type) {
    result.type = EXPR_BOOL;
    // EQT: false (0), NEQ: true (1)
    snprintf(result.value, sizeof(result.value), "%d",
             operator_type == TOKEN_NEQ ? 1 : 0);
    return result;
  }

  // String == string or string != string: use strcmp
  if ((operator_type == TOKEN_EQT || operator_type == TOKEN_NEQ) &&
      left.type == EXPR_STRING && right.type == EXPR_STRING) {
    int strcmp_id = context->temp_count++;
    int cmp_id = context->temp_count++;
    int ext_id = context->temp_count++;
    fprintf(context->file,
            "  %%tmp_%d = call i32 (i8*, i8*) @strcmp(i8* %s, i8* %s)\n",
            strcmp_id, left.value, right.value);
    const char *str_op = (operator_type == TOKEN_EQT) ? "icmp eq" : "icmp ne";
    fprintf(context->file, "  %%tmp_%d = %s i32 %%tmp_%d, 0\n", cmp_id, str_op,
            strcmp_id);
    fprintf(context->file, "  %%tmp_%d = zext i1 %%tmp_%d to i32\n", ext_id,
            cmp_id);
    result.type = EXPR_BOOL;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", ext_id);
    return result;
  }

  // String concatenation (+)
  if (left.type == EXPR_STRING || right.type == EXPR_STRING) {
    if (operator_type != TOKEN_ADD) {
      fprintf(stderr, "Error: Operator not supported for strings.\n");
      exit(EXIT_FAILURE);
    }
    
    ExprResult str_left = convert_to_string_expr(context, left);
    ExprResult str_right = convert_to_string_expr(context, right);

    int total_len = str_left.str_len + str_right.str_len;
    int malloc_id = context->temp_count++;
    int strcpy_id = context->temp_count++;
    int strcat_id = context->temp_count++;

    fprintf(context->file, "  %%tmp_%d = call i8* @malloc(i64 %d)\n", malloc_id,
            total_len + 1);
    fprintf(context->file,
            "  %%tmp_%d = call i8* @strcpy(i8* %%tmp_%d, i8* %s)\n", strcpy_id,
            malloc_id, str_left.value);
    fprintf(context->file,
            "  %%tmp_%d = call i8* @strcat(i8* %%tmp_%d, i8* %s)\n", strcat_id,
            malloc_id, str_right.value);

    result.type = EXPR_STRING;
    result.str_len = total_len;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", malloc_id);
    return result;
  }

  if (operator_type == TOKEN_AND || operator_type == TOKEN_OR) {
    int cmp_id = context->temp_count++;
    const char *op = (operator_type == TOKEN_AND) ? "and" : "or";
    fprintf(context->file, "  %%tmp_%d = %s i32 %s, %s\n", cmp_id, op,
            left.value, right.value);
    result.type = EXPR_BOOL;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", cmp_id);
    return result;
  }

  // Handle comparison operators — emit icmp (int) or fcmp (float)
  switch (operator_type) {
  case TOKEN_GT:
  case TOKEN_ST:
  case TOKEN_GE:
  case TOKEN_SE:
  case TOKEN_EQT:
  case TOKEN_NEQ: {
    int cmp_id = context->temp_count++;
    int ext_id = context->temp_count++;
    bool cmp_float = left.type == EXPR_FLOAT || right.type == EXPR_FLOAT;
    const char *cmp_op = NULL;
    if (cmp_float) {
      switch (operator_type) {
      case TOKEN_GT:
        cmp_op = "fcmp ogt";
        break;
      case TOKEN_ST:
        cmp_op = "fcmp olt";
        break;
      case TOKEN_GE:
        cmp_op = "fcmp oge";
        break;
      case TOKEN_SE:
        cmp_op = "fcmp ole";
        break;
      case TOKEN_EQT:
        cmp_op = "fcmp oeq";
        break;
      case TOKEN_NEQ:
        cmp_op = "fcmp one";
        break;
      default:
        break;
      }
      fprintf(context->file, "  %%tmp_%d = %s double %s, %s\n", cmp_id, cmp_op,
              left.value, right.value);
    } else {
      switch (operator_type) {
      case TOKEN_GT:
        cmp_op = "icmp sgt";
        break;
      case TOKEN_ST:
        cmp_op = "icmp slt";
        break;
      case TOKEN_GE:
        cmp_op = "icmp sge";
        break;
      case TOKEN_SE:
        cmp_op = "icmp sle";
        break;
      case TOKEN_EQT:
        cmp_op = "icmp eq";
        break;
      case TOKEN_NEQ:
        cmp_op = "icmp ne";
        break;
      default:
        break;
      }
      fprintf(context->file, "  %%tmp_%d = %s i32 %s, %s\n", cmp_id, cmp_op,
              left.value, right.value);
    }
    // Zero-extend i1 to i32 so the rest of the pipeline can use it uniformly
    fprintf(context->file, "  %%tmp_%d = zext i1 %%tmp_%d to i32\n", ext_id,
            cmp_id);
    result.type = EXPR_BOOL;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", ext_id);
    return result;
  }
  default:
    break;
  }

  int id = context->temp_count++;

  // If either side is a decimal, the whole operation must use LLVM double math.
  bool use_float = left.type == EXPR_FLOAT || right.type == EXPR_FLOAT;

  if (use_float) {
    char *left_value;
    char *right_value;

    // Convert integer operands to double before mixed int/float arithmetic.
    if (left.type == EXPR_INT) {
      int cast_id = context->temp_count++;
      fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id,
              left.value);
      int size = snprintf(NULL, 0, "%%tmp_%d", cast_id) + 1;
      left_value = alloc_space(size, sizeof(char));
      snprintf(left_value, size, "%%tmp_%d", cast_id);
    } else {
      int size = strlen(left.value) + 1;
      left_value = alloc_space(size, sizeof(char));
      snprintf(left_value, size, "%s", left.value);
    }

    // Convert the right side too if it is the integer part of a mixed
    // expression.
    if (right.type == EXPR_INT) {
      int cast_id = context->temp_count++;
      fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id,
              right.value);
      int size = snprintf(NULL, 0, "%%tmp_%d", cast_id) + 1;
      right_value = alloc_space(size, sizeof(char));
      snprintf(right_value, size, "%%tmp_%d", cast_id);
    } else {
      int size = strlen(right.value) + 1;
      right_value = alloc_space(size, sizeof(char));
      snprintf(right_value, size, "%s", right.value);
    }

    // Choose the LLVM floating-point instruction for this Apollo operator.
    const char *llvm_op = NULL;

    switch (operator_type) {
    case TOKEN_ADD:
      llvm_op = "fadd";
      break;
    case TOKEN_SUB:
      llvm_op = "fsub";
      break;
    case TOKEN_MUL:
      llvm_op = "fmul";
      break;
    case TOKEN_DIV:
      llvm_op = "fdiv";
      break;
    case TOKEN_MOD:
      llvm_op = "frem";
      break;
    default:
      fprintf(stderr, "Invalid float operator.\n");
      exit(EXIT_FAILURE);
    }

    fprintf(context->file, "%%tmp_%d = %s double %s, %s\n", id, llvm_op,
            left_value, right_value);

    free(left_value);
    free(right_value);

    result.type = EXPR_FLOAT;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", id);

    return result;
  }

  // If both sides are integers, use LLVM integer arithmetic instructions.
  const char *llvm_op = NULL;

  switch (operator_type) {
  case TOKEN_ADD:
    llvm_op = "add";
    break;
  case TOKEN_SUB:
    llvm_op = "sub";
    break;
  case TOKEN_MUL:
    llvm_op = "mul";
    break;
  case TOKEN_DIV:
    llvm_op = "sdiv";
    break;
  case TOKEN_MOD:
    llvm_op = "srem";
    break;
  default:
    fprintf(stderr, "Invalid integer operator.\n");
    exit(EXIT_FAILURE);
  }

  if(strcmp(llvm_op, "sdiv") == 0 && strcmp(right.value, "0") == 0 ){
    fprintf(stderr, "CRITICAL: CANNOT DIVIDE BY ZERO.\n");
    exit(EXIT_FAILURE);
  }
  fprintf(context->file, "%%tmp_%d = %s i32 %s, %s\n", id, llvm_op, left.value,
          right.value);

  result.type = EXPR_INT;
  snprintf(result.value, sizeof(result.value), "%%tmp_%d", id);

  return result;
}

// Emits LLVM that prints the final result of a parsed expression.
// The ExprResult type decides whether printf receives an i32 or a double.
void gen_println_expr(CodegenContext *context, ExprResult result) {
  int id = context->string_constant_count++;

  if (result.type == EXPR_BOOL) {
    int tmp_id = context->temp_count++;

    // Allocate "true\n\0" and "false\n\0"
    fprintf(context->file, "  %%cmp_true_%d = alloca [6 x i8]\n", id);
    fprintf(context->file,
            "  store [6 x i8] c\"true\\0A\\00\", [6 x i8]* %%cmp_true_%d\n",
            id);
    fprintf(context->file,
            "  %%cmp_true_ptr_%d = getelementptr inbounds [6 x i8], [6 x i8]* "
            "%%cmp_true_%d, i32 0, i32 0\n",
            id, id);

    fprintf(context->file, "  %%cmp_false_%d = alloca [7 x i8]\n", id);
    fprintf(context->file,
            "  store [7 x i8] c\"false\\0A\\00\", [7 x i8]* %%cmp_false_%d\n",
            id);
    fprintf(context->file,
            "  %%cmp_false_ptr_%d = getelementptr inbounds [7 x i8], [7 x i8]* "
            "%%cmp_false_%d, i32 0, i32 0\n",
            id, id);

    // Branch on value == 1
    fprintf(context->file, "  %%cmp_check_%d = icmp eq i32 %s, 1\n", id,
            result.value);
    fprintf(context->file,
            "  br i1 %%cmp_check_%d, label %%cmp_true_lbl_%d, label "
            "%%cmp_false_lbl_%d\n\n",
            id, id, id);

    fprintf(context->file, "cmp_true_lbl_%d:\n", id);
    fprintf(context->file,
            "  call i32 (i8*, ...) @printf(i8* %%cmp_true_ptr_%d)\n", id);
    fprintf(context->file, "  br label %%cmp_end_%d\n\n", id);

    fprintf(context->file, "cmp_false_lbl_%d:\n", id);
    fprintf(context->file,
            "  call i32 (i8*, ...) @printf(i8* %%cmp_false_ptr_%d)\n", id);
    fprintf(context->file, "  br label %%cmp_end_%d\n\n", id);

    fprintf(context->file, "cmp_end_%d:\n", id);
    return;
  }

  if (result.type == EXPR_STRING) {
    fprintf(context->file, "; Print expression string\n");
    fprintf(context->file, "%%str_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file,
            "store [4 x i8] c\"%%s\\0A\\00\", [4 x i8]* %%str_fmt_%d\n", id);
    fprintf(context->file,
            "%%str_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
            "%%str_fmt_%d, i32 0, i32 0\n",
            id, id);
    fprintf(context->file,
            "call i32 (i8*, ...) @printf(i8* %%str_fmt_ptr_%d, i8* %s)\n\n", id,
            result.value);
    return;
  }

  if (result.type == EXPR_FLOAT) {
    fprintf(context->file, "%%float_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file,
            "store [4 x i8] c\"%%f\\0A\\00\", [4 x i8]* %%float_fmt_%d\n", id);
    fprintf(context->file,
            "%%float_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
            "%%float_fmt_%d, i32 0, i32 0\n",
            id, id);
    fprintf(
        context->file,
        "call i32 (i8*, ...) @printf(i8* %%float_fmt_ptr_%d, double %s)\n\n",
        id, result.value);
    return;
  }

  fprintf(context->file, "%%int_fmt_%d = alloca [4 x i8]\n", id);
  fprintf(context->file,
          "store [4 x i8] c\"%%d\\0A\\00\", [4 x i8]* %%int_fmt_%d\n", id);
  fprintf(context->file,
          "%%int_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
          "%%int_fmt_%d, i32 0, i32 0\n",
          id, id);
  fprintf(context->file,
          "call i32 (i8*, ...) @printf(i8* %%int_fmt_ptr_%d, i32 %s)\n\n", id,
          result.value);
}

static void gen_println_bool_var(CodegenContext *context, Symbol *sym) {
  int id = context->string_constant_count++;
  int temp_id = context->temp_count++;

  // Load the i8 value from the variable
  fprintf(context->file, "; Load bool variable and print as true/false/null\n");
  fprintf(context->file, "  %%tmp_%d = load i8, i8* %%%s\n", temp_id,
          sym->llvm_name);

  // Allocate "true\n\0", "false\n\0", and "null\n\0"
  fprintf(context->file, "  %%bool_true_%d = alloca [6 x i8]\n", id);
  fprintf(context->file,
          "  store [6 x i8] c\"true\\0A\\00\", [6 x i8]* %%bool_true_%d\n", id);
  fprintf(context->file,
          "  %%bool_true_ptr_%d = getelementptr inbounds [6 x i8], [6 x i8]* "
          "%%bool_true_%d, i32 0, i32 0\n",
          id, id);

  fprintf(context->file, "  %%bool_false_%d = alloca [7 x i8]\n", id);
  fprintf(context->file,
          "  store [7 x i8] c\"false\\0A\\00\", [7 x i8]* %%bool_false_%d\n",
          id);
  fprintf(context->file,
          "  %%bool_false_ptr_%d = getelementptr inbounds [7 x i8], [7 x i8]* "
          "%%bool_false_%d, i32 0, i32 0\n",
          id, id);

  fprintf(context->file, "  %%bool_null_%d = alloca [6 x i8]\n", id);
  fprintf(context->file,
          "  store [6 x i8] c\"null\\0A\\00\", [6 x i8]* %%bool_null_%d\n", id);
  fprintf(context->file,
          "  %%bool_null_ptr_%d = getelementptr inbounds [6 x i8], [6 x i8]* "
          "%%bool_null_%d, i32 0, i32 0\n",
          id, id);

  // Compare loaded value with 2 (null)
  int is_null_id = context->temp_count++;
  int is_true_id = context->temp_count++;

  fprintf(context->file, "  %%tmp_%d = icmp eq i8 %%tmp_%d, 2\n", is_null_id,
          temp_id);
  fprintf(context->file,
          "  br i1 %%tmp_%d, label %%bool_null_lbl_%d, label "
          "%%bool_not_null_lbl_%d\n\n",
          is_null_id, id, id);

  fprintf(context->file, "bool_not_null_lbl_%d:\n", id);
  fprintf(context->file, "  %%tmp_%d = icmp eq i8 %%tmp_%d, 1\n", is_true_id,
          temp_id);
  fprintf(context->file,
          "  br i1 %%tmp_%d, label %%bool_true_lbl_%d, label "
          "%%bool_false_lbl_%d\n\n",
          is_true_id, id, id);

  fprintf(context->file, "bool_null_lbl_%d:\n", id);
  fprintf(context->file,
          "  call i32 (i8*, ...) @printf(i8* %%bool_null_ptr_%d)\n", id);
  fprintf(context->file, "  br label %%bool_end_%d\n\n", id);

  fprintf(context->file, "bool_true_lbl_%d:\n", id);
  fprintf(context->file,
          "  call i32 (i8*, ...) @printf(i8* %%bool_true_ptr_%d)\n", id);
  fprintf(context->file, "  br label %%bool_end_%d\n\n", id);

  fprintf(context->file, "bool_false_lbl_%d:\n", id);
  fprintf(context->file,
          "  call i32 (i8*, ...) @printf(i8* %%bool_false_ptr_%d)\n", id);
  fprintf(context->file, "  br label %%bool_end_%d\n\n", id);

  fprintf(context->file, "bool_end_%d:\n", id);
}

void gen_println_variable(CodegenContext *context, Symbol *sym, int scope_level) {
  int is_global = (sym->scope_level == 0 && (!sym->fxn || strcmp(sym->fxn->name, "global") == 0));

  if (sym->type == TYPE_STRING) {
    int id = context->string_constant_count++;
    int temp_id = context->temp_count++;
    fprintf(context->file, "; Print string variable\n");
    fprintf(context->file, "%%str_fmt_%d = alloca [4 x i8]\n", id);
    fprintf(context->file,
            "store [4 x i8] c\"%%s\\0A\\00\", [4 x i8]* %%str_fmt_%d\n", id);
    fprintf(context->file,
            "%%str_fmt_ptr_%d = getelementptr inbounds [4 x i8], [4 x i8]* "
            "%%str_fmt_%d, i32 0, i32 0\n",
            id, id);
    if (is_global) {
      fprintf(context->file, "  %%str_val_load_%d = load i8*, i8** @%s_val\n",
              temp_id, sym->llvm_name);
    } else {
      fprintf(context->file, "  %%str_val_load_%d = load i8*, i8** %%%s_val\n",
              temp_id, sym->llvm_name);
    }
    fprintf(context->file,
            "call i32 (i8*, ...) @printf(i8* %%str_fmt_ptr_%d, i8* "
            "%%str_val_load_%d)\n\n",
            id, temp_id);

    return;
  }
  const char *llvm_type = llvm_datatype(sym->type);
  int temp_id = context->temp_count++;

  fprintf(context->file, "; Load variable value\n");
  if (is_global) {
    fprintf(context->file, "  %%tmp_%d = load %s, %s* @%s\n", temp_id, llvm_type,
            llvm_type, sym->llvm_name);
  } else {
    fprintf(context->file, "  %%tmp_%d = load %s, %s* %%%s\n", temp_id, llvm_type,
            llvm_type, sym->llvm_name);
  }

  if (sym->type == TYPE_FLOAT || sym->type == TYPE_INT) {
    ExprResult result;
    result.type = (sym->type == TYPE_FLOAT) ? EXPR_FLOAT : EXPR_INT;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", temp_id);

    gen_println_expr(context, result);

  } else {
    gen_println_bool_var(context, sym);
    return;
  }
}

/*--------------------------------------------------------------------------------

                  ARITHMETICS SEGMENT -> END

---------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------

                  CONTROL FLOW -> BEGIN

---------------------------------------------------------------------------------*/

void gen_while_from_ast(CodegenContext *context, ASTNode *while_node) {
  int label_id = context->temp_count++;

  fprintf(context->file, "  br label %%while_cond_%d\n\n", label_id);

  fprintf(context->file, "while_cond_%d:\n", label_id);
  ExprResult cond_res =
      gen_expr_from_ast(context, while_node->while_lp.condition);
      
  const char *cond_val = cond_res.value;
  if (strcmp(cond_val, "true") == 0) cond_val = "1";
  else if (strcmp(cond_val, "false") == 0) cond_val = "0";

  int trunc_id = context->temp_count++;
  fprintf(context->file, "  %%tmp_%d = trunc i32 %s to i1\n", trunc_id,
          cond_val);

  fprintf(context->file,
          "  br i1 %%tmp_%d, label %%while_body_%d, label %%while_end_%d\n\n",
          trunc_id, label_id, label_id);

  fprintf(context->file, "while_body_%d:\n", label_id);
  gen_block_from_ast(context, while_node->while_lp.then_block);
  fprintf(context->file, "  br label %%while_cond_%d\n\n", label_id);

  fprintf(context->file, "while_end_%d:\n", label_id);
}

void gen_for_from_ast(CodegenContext *context, ASTNode *for_node) {
  if (for_node == NULL || for_node->Type != AST_FOR)
    return;

  int label_id = context->temp_count++;

  // 1. Initializer: variable
  if (for_node->for_lp.variable != NULL) {
    if (for_node->for_lp.variable->Type == AST_VAR_DECL) {
      gen_var_decl_from_ast(context, for_node->for_lp.variable);
    } else if (for_node->for_lp.variable->Type == AST_VAR_ASS) {
      gen_var_assign_from_ast(context, for_node->for_lp.variable);
    }
  }

  // Branch to condition
  fprintf(context->file, "  br label %%for_cond_%d\n\n", label_id);

  // 2. Condition block
  fprintf(context->file, "for_cond_%d:\n", label_id);

  int trunc_id = context->temp_count++;
  if (for_node->for_lp.condtion != NULL) {
    ExprResult cond_res = gen_expr_from_ast(context, for_node->for_lp.condtion);
    
    const char *cond_val = cond_res.value;
    if (strcmp(cond_val, "true") == 0) cond_val = "1";
    else if (strcmp(cond_val, "false") == 0) cond_val = "0";

    fprintf(context->file, "  %%tmp_%d = trunc i32 %s to i1\n", trunc_id,
            cond_val);
    fprintf(context->file,
            "  br i1 %%tmp_%d, label %%for_body_%d, label %%for_end_%d\n\n",
            trunc_id, label_id, label_id);
  } else {
    // If no condition, default to true
    fprintf(context->file, "  br label %%for_body_%d\n\n", label_id);
  }

  // 3. Body block
  fprintf(context->file, "for_body_%d:\n", label_id);
  if (for_node->for_lp.then_block != NULL) {
    gen_block_from_ast(context, for_node->for_lp.then_block);
  }

  fprintf(context->file, "  br label %%for_step_%d\n\n", label_id);

  // 4. Step/Update block: var_operation
  fprintf(context->file, "for_step_%d:\n", label_id);
  if (for_node->for_lp.var_operation != NULL) {
    if (for_node->for_lp.var_operation->Type == AST_VAR_ASS) {
      gen_var_assign_from_ast(context, for_node->for_lp.var_operation);
    }
  }
  fprintf(context->file, "  br label %%for_cond_%d\n\n", label_id);

  // 5. End block
  fprintf(context->file, "for_end_%d:\n", label_id);
}

void gen_fxn_call_from_ast(CodegenContext *context, ASTNode *call_node) {
  if (call_node == NULL || call_node->Type != AST_CALL_FXN)
    return;

  const char *func_name = call_node->call_fxn.resolved_symbol->llvm_name;

  // Evaluate all argument expressions and collect results
  ExprResult arg_results[32];
  DataType   arg_types[32];
  int arg_count = 0;
  Args *arg = call_node->call_fxn.args;
  Params *param = NULL;
  if (call_node->call_fxn.resolved_symbol && call_node->call_fxn.resolved_symbol->fxn) {
    param = call_node->call_fxn.resolved_symbol->fxn->params;
  }
  while (arg && arg_count < 32) {
    arg_results[arg_count] = gen_expr_from_ast(context, arg->arg);
    arg_types[arg_count] = (param) ? param->param->var_decl.value_type : TYPE_INT;
    arg_count++;
    arg   = arg->next;
    if (param) param = param->next;
  }

  if (call_node->call_fxn.return_type == TYPE_NULL) {
    fprintf(context->file, "  call void @%s(", func_name);
  } else {
    const char *llvm_type = llvm_datatype(call_node->call_fxn.return_type);
    if (llvm_type == NULL) llvm_type = "void";
    int temp_id = context->temp_count++;
    fprintf(context->file, "  %%tmp_%d = call %s @%s(", temp_id, llvm_type, func_name);
  }
  // Emit argument list
  for (int i = 0; i < arg_count; i++) {
    const char *atype = llvm_datatype(arg_types[i]);
    if (atype == NULL) atype = "i32";
    fprintf(context->file, "%s %s", atype, arg_results[i].value);
    if (i < arg_count - 1) fprintf(context->file, ", ");
  }
  fprintf(context->file, ")\n");
}

void gen_if_from_ast(CodegenContext *context, ASTNode *if_node) {
  int label_id = context->temp_count++;

  // 1. Evaluate condition
  ExprResult cond_res = gen_expr_from_ast(context, if_node->if_stmt.condition);
  
  const char *cond_val = cond_res.value;
  if (strcmp(cond_val, "true") == 0) cond_val = "1";
  else if (strcmp(cond_val, "false") == 0) cond_val = "0";

  int trunc_id = context->temp_count++;
  fprintf(context->file, "  %%tmp_%d = trunc i32 %s to i1\n", trunc_id,
          cond_val);

  // 2. Conditional branch
  fprintf(context->file,
          "  br i1 %%tmp_%d, label %%if_true_%d, label %%if_false_%d\n\n",
          trunc_id, label_id, label_id);

  // 3. Emit True Block and generate its statements recursively
  fprintf(context->file, "if_true_%d:\n", label_id);
  gen_block_from_ast(context, if_node->if_stmt.then_block);
  fprintf(context->file, "  br label %%if_end_%d\n\n", label_id);

  // 4. Emit False Block (elif, else, or empty)
  fprintf(context->file, "if_false_%d:\n", label_id);
  if (if_node->if_stmt.else_block != NULL) {
    if (if_node->if_stmt.else_block->Type == AST_IF) {
      gen_if_from_ast(context, if_node->if_stmt.else_block); // Recursive Elif
    } else {
      gen_block_from_ast(context, if_node->if_stmt.else_block); // Else block
    }
  }
  fprintf(context->file, "  br label %%if_end_%d\n\n", label_id);

  // 5. Emit End Block
  fprintf(context->file, "if_end_%d:\n", label_id);
}

/*--------------------------------------------------------------------------------

                  CONTROL FLOW -> END

---------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------

          -------------COMPLETE END-----------------

---------------------------------------------------------------------------------*/

// Emits the return instruction, closes the current LLVM function, and closes
// the file.
void gen_function_end(CodegenContext *context, bool is_main) {
  (is_main) ? fprintf(context->file, "ret i32 0\n")
            : fprintf(context->file, "    ret void\n");

  fprintf(context->file, "}\n");
}


/*--------------------------------------------------------------------------------

          -------------HELPERS-----------------

---------------------------------------------------------------------------------*/

