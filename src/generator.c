/*--------------------------------------------------------------------------------

                         THE TRANSLATOR

---------------------------------------------------------------------------------*/

#include "../headers/arithmetic.h"
#include "../headers/ast.h"
#include "../headers/token.h"
#include "../headers/variables.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Symbol *lookup_variable(CodegenContext *context, const char *name);
static void gen_println_bool(CodegenContext *context, Token token);
static void create_var_from_expr_result(CodegenContext *context,
                                        ExprResult result, const char *name);

/*--------------------------------------------------------------------------------

                  INITIALIZING SEGMENT -> BEGIN

---------------------------------------------------------------------------------*/

// Opens the LLVM output file and resets the counters used for generated names.
void codegen_init(CodegenContext *context, const char *output_filename) {
  context->file = fopen(output_filename, "w");

  context->string_constant_count = 0;
  context->temp_count = 0;

  if (context->file == NULL) {
    fprintf(stderr, "Could not create output file %s\n", output_filename);
    exit(1);
  }

  // Print headers and link C's native printf function for our standard I/O
  fprintf(context->file, "; --- Apollo Native Compiler Backend Output ---\n");

  fprintf(context->file, "declare i8* @printf(i8*, ...)\n\n");
  fprintf(context->file, "declare i8* @malloc(i64)\n");
  fprintf(context->file, "declare i8* @strcat(i8*, i8*)\n");
  fprintf(context->file, "declare i8* @strcpy(i8*, i8*)\n");

}

// Emits the LLVM function header. Apollo's run() function becomes LLVM's
// main().
void gen_function_start(CodegenContext *context, const char *name) {
  // if it run then make that the main function.
  if (strcmp(name, "run") == 0) {
    fprintf(context->file, "define i32 @main() {\n");
  } else {
    fprintf(context->file, "define void @%s()", name);
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
      exit(1);
    }

    return;
  }

  if (value->Type == AST_BINARY_EXPR) {
    ExprResult result = gen_expr_from_ast(context, value);
    gen_println_expr(context, result);
    return;
  }
  if (value->Type == AST_VAR_REF) {
    char var_name[64];
    snprintf(var_name, sizeof(var_name), "%.*s", value->var_ref.name_length,
             value->var_ref.name);
    gen_println_variable(context, var_name);

    return;
  }

  fprintf(stderr, "Unsupported value in println AST.\n");
  exit(1);
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

static const char *llvm_datatype(datatype type) {
  switch (type) {
  case TYPE_INT:
    return "i32";

  case TYPE_FLOAT:
    return "double";

  case TYPE_BOOL:
  case TYPE_NULL:
    return "i8";

  case TYPE_CHAR:
    return "i8";

  default:
    return NULL;
  }
}


static void create_string_var(CodegenContext *context, const char *value_start, int length, const char *llvm_name) {
  int id = context->string_constant_count++;
  int internal_len = length - 1; // removing the quotes
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
  fprintf(context->file, "  %%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* %%str_loc_%d, i32 0, i32 0\n",
          id, llvm_len, llvm_len, id);
  fprintf(context->file, "  store i8* %%str_ptr_%d, i8** %%%s_val\n", id, llvm_name);
  fprintf(context->file, "  store i32 %d, i32* %%%s_len\n\n", internal_len, llvm_name);
}





// Emits LLVM for a literal variable declaration.
// Example: var age = 45; becomes an alloca slot plus a store into that slot.
void create_var(CodegenContext *context, const char *number_start, int length,
                const char *name) {

  Symbol *sym = lookup_variable(context, name);
  char *llvm_name = sym->llvm_name;

  fprintf(context->file, "; Allocate integer variable slot\n");

  const char *llvm_type = llvm_datatype(sym->type);
  if (llvm_type == NULL) {
    create_string_var(context, number_start, length, llvm_name);

  } else if (sym->type == TYPE_BOOL || sym->type == TYPE_NULL) {
    fprintf(context->file, "%%%s = alloca i8\n", llvm_name);
    int val = 2; // Default to null
    if (strncmp(number_start, "true", length) == 0) val = 1;
    else if (strncmp(number_start, "false", length) == 0) val = 0;
    fprintf(context->file, "store i8 %d, i8* %%%s\n\n", val, llvm_name);

  } else {
    fprintf(context->file, "%%%s = alloca %s\n", llvm_name, llvm_type);
    fprintf(context->file, "store %s %.*s, %s* %%%s\n\n", llvm_type, length,
            number_start, llvm_type, llvm_name);
  }
}

static void create_var_from_expr_result(CodegenContext *context,
                                        ExprResult result, const char *name) {
  Symbol *sym = lookup_variable(context, name);
  char *llvm_name = sym->llvm_name;

  if (result.type == EXPR_FLOAT) {
    fprintf(context->file, "; Allocate decimal expression variable slot\n");
    fprintf(context->file, "%%%s = alloca double\n", llvm_name);
    fprintf(context->file, "store double %s, double* %%%s\n\n", result.value,
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
    exit(1);
  }

  ASTNode *value = var_node->var_decl.value;

  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "%.*s", var_node->var_decl.name_length,
           var_node->var_decl.name);

  if (value->Type == AST_LITERAL_EXPR) {
    Token token = value->literal_expr.token;
    // Register here — type is known directly from the literal token
    datatype lit_type = (token.type == TOKEN_FLOAT) ? TYPE_FLOAT : TYPE_INT;
    register_variable(context, name_buf, lit_type);
    create_var(context, token.start, token.length, name_buf);
    return;
  }

  if (value->Type == AST_BINARY_EXPR || value->Type == AST_VAR_REF) {
    ExprResult result = gen_expr_from_ast(context, value);
    // Infer type from the expression result and register the variable now
    datatype inferred = (result.type == EXPR_FLOAT) ? TYPE_FLOAT : TYPE_INT;
    register_variable(context, name_buf, inferred);
    create_var_from_expr_result(context, result, name_buf);
    return;
  }

  fprintf(
      stderr,
      "Expected literal or arithmetic expression in variable declaration.\n");
  exit(1);
}


void gen_var_assign_from_ast(CodegenContext *context, ASTNode *assign_node) {
  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "%.*s", 
           assign_node->var_assign.name_length, 
           assign_node->var_assign.name);
           
  Symbol *sym = lookup_variable(context, name_buf);
  if (!sym) {
    fprintf(stderr, "Error: Variable '%s' not declared.\n", name_buf);
    exit(1);
  }

  ASTNode *value = assign_node->var_assign.value;


  if (value->Type == AST_VAR_REF) {
    char rhs_name[64];
    snprintf(rhs_name, sizeof(rhs_name), "%.*s", value->var_ref.name_length, value->var_ref.name);
    Symbol *rhs_sym = lookup_variable(context, rhs_name);
    
    if (rhs_sym) {
      if (rhs_sym->type == TYPE_STRING) {
        int temp_ptr = context->temp_count++;
        int temp_len = context->temp_count++;
        fprintf(context->file, "  %%tmp_%d = load i8*, i8** %%%s_val\n", temp_ptr, rhs_sym->llvm_name);
        fprintf(context->file, "  %%tmp_%d = load i32, i32* %%%s_len\n", temp_len, rhs_sym->llvm_name);
        fprintf(context->file, "  store i8* %%tmp_%d, i8** %%%s_val\n", temp_ptr, sym->llvm_name);
        fprintf(context->file, "  store i32 %%tmp_%d, i32* %%%s_len\n\n", temp_len, sym->llvm_name);
        return;
      }
      if (rhs_sym->type == TYPE_BOOL) {
        int temp_bool = context->temp_count++;
        fprintf(context->file, "  %%tmp_%d = load i8, i8* %%%s\n", temp_bool, rhs_sym->llvm_name);
        fprintf(context->file, "  store i8 %%tmp_%d, i8* %%%s\n\n", temp_bool, sym->llvm_name);
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

      // If the variable was declared as null, its _val/_len slots don't exist yet.
      // Allocate them now and upgrade the symbol type to TYPE_STRING.
      if (sym->type == TYPE_NULL) {
        fprintf(context->file, "  %%%s_val = alloca i8*\n", sym->llvm_name);
        fprintf(context->file, "  %%%s_len = alloca i32\n", sym->llvm_name);
        sym->type = TYPE_STRING;
      }

      fprintf(context->file, "  %%str_loc_%d = alloca [%d x i8]\n", id, llvm_len);
      fprintf(context->file, "  store [%d x i8] c\"", llvm_len);
      emit_llvm_string_contents(context->file, token.start + 1, internal_len);
      fprintf(context->file, "\\00\", [%d x i8]* %%str_loc_%d\n", llvm_len, id);

      fprintf(context->file, "  %%str_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* %%str_loc_%d, i32 0, i32 0\n",
              id, llvm_len, llvm_len, id);

      fprintf(context->file, "  store i8* %%str_ptr_%d, i8** %%%s_val\n", id, sym->llvm_name);
      fprintf(context->file, "  store i32 %d, i32* %%%s_len\n\n", internal_len, sym->llvm_name);
      return;
    } else if (token.type == TOKEN_BOOL || token.type == TOKEN_NULL) {
      int val = 2; // Default to null
      if (token.type == TOKEN_BOOL) {
        val = (strncmp(token.start, "true", token.length) == 0) ? 1 : 0;
      }
      fprintf(context->file, "  store i8 %d, i8* %%%s\n\n", val, sym->llvm_name);
      return;
    }
  }

  ExprResult result = gen_expr_from_ast(context, value);

  if (result.type == EXPR_FLOAT) {
    // If null, the existing i8 slot can't hold a double — allocate a new slot.
    if (sym->type == TYPE_NULL) {
      char new_name[72];
      snprintf(new_name, sizeof(new_name), "%s_fslot", sym->llvm_name);
      fprintf(context->file, "  %%%s = alloca double\n", new_name);
      snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", new_name);
      sym->type = TYPE_FLOAT;
    }
    fprintf(context->file, "  store double %s, double* %%%s\n\n", result.value, sym->llvm_name);
  } else if (result.type == EXPR_INT) {
    if (sym->type == TYPE_BOOL) {
      fprintf(context->file, "  store i8 %s, i8* %%%s\n", result.value, sym->llvm_name);
    } else if (sym->type == TYPE_NULL) {
      // Null was allocated as i8 — allocate a fresh i32 slot and upgrade the symbol.
      char new_name[72];
      snprintf(new_name, sizeof(new_name), "%s_islot", sym->llvm_name);
      fprintf(context->file, "  %%%s = alloca i32\n", new_name);
      snprintf(sym->llvm_name, sizeof(sym->llvm_name), "%s", new_name);
      sym->type = TYPE_INT;
      fprintf(context->file, "  store i32 %s, i32* %%%s\n\n", result.value, sym->llvm_name);
    } else {
      fprintf(context->file, "  store i32 %s, i32* %%%s\n", result.value, sym->llvm_name);
    }
  }

}



Symbol *lookup_variable(CodegenContext *context, const char *name) {
  for (int i = 0; i < context->symbol_count; i++) {
    if (strcmp(context->symbols[i].name, name) == 0) {
      return &context->symbols[i];
    }
  }
  return NULL;
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
      int llvm_len = internal_len + 1; // include null terminator

      // 1. Allocate the local stack string buffer
      fprintf(context->file, "  %%str_expr_%d = alloca [%d x i8]\n", id, llvm_len);
      fprintf(context->file, "  store [%d x i8] c\"", llvm_len);
      emit_llvm_string_contents(context->file, token.start + 1, internal_len);
      fprintf(context->file, "\\00\", [%d x i8]* %%str_expr_%d\n", llvm_len, id);

      // 2. Get its pointer
      int temp_id = context->temp_count++;
      fprintf(context->file, "  %%str_expr_ptr_%d = getelementptr inbounds [%d x i8], [%d x i8]* %%str_expr_%d, i32 0, i32 0\n",
              temp_id, llvm_len, llvm_len, id);

      ExprResult result;
      result.type = EXPR_STRING;
      result.str_len = internal_len;
      snprintf(result.value, sizeof(result.value), "%%str_expr_ptr_%d", temp_id);
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
    char name[64];
    snprintf(name, sizeof(name), "%.*s", expr->var_ref.name_length,
             expr->var_ref.name);
    Symbol *sym = lookup_variable(context, name);
    int temp_id = context->temp_count++;

    if (sym->type == TYPE_STRING) {
      fprintf(context->file, "  %%tmp_%d = load i8*, i8** %%%s_val\n", temp_id, sym->llvm_name);
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

  default:
    fprintf(stderr, "Unsupported expression AST node.\n");
    exit(1);
  }
}

ExprResult make_literal_expr(Token token) {
  ExprResult result;

  if (token.type == TOKEN_FLOAT) {
    result.type = EXPR_FLOAT;
  } else {
    result.type = EXPR_INT;
  }

  snprintf(result.value, sizeof(result.value), "%.*s", token.length,
           token.start);

  return result;
}

// Emits LLVM for a binary arithmetic expression such as left + right.
// Returns a new ExprResult pointing at the generated temporary value.
ExprResult gen_binary_expr(CodegenContext *context, ExprResult left,
                           TokenType operator_type, ExprResult right) {
  ExprResult result;

  // Handle string concatenation
  if (left.type == EXPR_STRING || right.type == EXPR_STRING) {
    if (operator_type != TOKEN_ADD) {
      fprintf(stderr, "Error: Operator not supported for strings.\n");
      exit(1);
    }
    int total_len = left.str_len + right.str_len;
    int malloc_id = context->temp_count++;
    int strcpy_id = context->temp_count++;
    int strcat_id = context->temp_count++;

    // 1. malloc a new buffer
    fprintf(context->file, "  %%tmp_%d = call i8* @malloc(i64 %d)\n", malloc_id, total_len + 1);
    // 2. strcpy left string
    fprintf(context->file, "  %%tmp_%d = call i8* @strcpy(i8* %%tmp_%d, i8* %s)\n", strcpy_id, malloc_id, left.value);
    // 3. strcat right string
    fprintf(context->file, "  %%tmp_%d = call i8* @strcat(i8* %%tmp_%d, i8* %s)\n", strcat_id, malloc_id, right.value);

    result.type = EXPR_STRING;
    result.str_len = total_len;
    snprintf(result.value, sizeof(result.value), "%%tmp_%d", malloc_id);
    return result;
  }

  int id = context->temp_count++;

  // If either side is a decimal, the whole operation must use LLVM double math.
  bool use_float = left.type == EXPR_FLOAT || right.type == EXPR_FLOAT;

  if (use_float) {
    char left_value[64];
    char right_value[64];

    // Convert integer operands to double before mixed int/float arithmetic.
    if (left.type == EXPR_INT) {
      int cast_id = context->temp_count++;
      fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id,
              left.value);
      snprintf(left_value, sizeof(left_value), "%%tmp_%d", cast_id);
    } else {
      snprintf(left_value, sizeof(left_value), "%s", left.value);
    }

    // Convert the right side too if it is the integer part of a mixed
    // expression.
    if (right.type == EXPR_INT) {
      int cast_id = context->temp_count++;
      fprintf(context->file, "%%tmp_%d = sitofp i32 %s to double\n", cast_id,
              right.value);
      snprintf(right_value, sizeof(right_value), "%%tmp_%d", cast_id);
    } else {
      snprintf(right_value, sizeof(right_value), "%s", right.value);
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
      exit(1);
    }

    fprintf(context->file, "%%tmp_%d = %s double %s, %s\n", id, llvm_op,
            left_value, right_value);

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
    exit(1);
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
            "call i32 (i8*, ...) @printf(i8* %%str_fmt_ptr_%d, i8* %s)\n\n",
            id, result.value);
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
          "  store [7 x i8] c\"false\\0A\\00\", [7 x i8]* %%bool_false_%d\n", id);
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

  fprintf(context->file, "  %%tmp_%d = icmp eq i8 %%tmp_%d, 2\n", is_null_id, temp_id);
  fprintf(context->file, "  br i1 %%tmp_%d, label %%bool_null_lbl_%d, label %%bool_not_null_lbl_%d\n\n",
          is_null_id, id, id);

  fprintf(context->file, "bool_not_null_lbl_%d:\n", id);
  fprintf(context->file, "  %%tmp_%d = icmp eq i8 %%tmp_%d, 1\n", is_true_id, temp_id);
  fprintf(context->file, "  br i1 %%tmp_%d, label %%bool_true_lbl_%d, label %%bool_false_lbl_%d\n\n",
          is_true_id, id, id);

  fprintf(context->file, "bool_null_lbl_%d:\n", id);
  fprintf(context->file, "  call i32 (i8*, ...) @printf(i8* %%bool_null_ptr_%d)\n", id);
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

void gen_println_variable(CodegenContext *context, char *name) {
  Symbol *sym = lookup_variable(context, name);

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
    fprintf(context->file,
            "  %%str_val_load_%d = load i8*, i8** %%%s_val\n",
            temp_id, sym->llvm_name);
    fprintf(context->file,
            "call i32 (i8*, ...) @printf(i8* %%str_fmt_ptr_%d, i8* "
            "%%str_val_load_%d)\n\n",
            id, temp_id);

    return;
  }
  const char *llvm_type = llvm_datatype(sym->type);
  int temp_id = context->temp_count++;

  fprintf(context->file, "; Load variable value\n");
  fprintf(context->file, "  %%tmp_%d = load %s, %s* %%%s\n", temp_id, llvm_type,
          llvm_type, sym->llvm_name);

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
  fclose(context->file);
}
