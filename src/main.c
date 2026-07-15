/*Coordinator*/

#include "../headers/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../headers/error.h"
#include "../headers/ast.h"
#include "../headers/semantic.h"
#include "../headers/defs.h"

ASTNode *compile_parse(Lexer *lexer, ErrorStack *s, CodegenContext *context);

static char *read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");

  if (file == NULL) {
    fprintf(stderr, "[DRIVER] Could not open source file: %s\n", filename);
    exit(EXIT_FAILURE);
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *source = malloc(file_size + 1);
  if (source == NULL) {
    fprintf(stderr, "[DRIVER] Could not allocate memory for source file.\n");
    fclose(file);
    exit(EXIT_FAILURE);
  }

  size_t bytes_read = fread(source, 1, file_size, file);
  source[bytes_read] = '\0';

  fclose(file);
  return source;
}

// Automatically runs the program after successfully compilation.
void llvm_compilation() {
  int result = system("clang -O3 temp\\output.bc -o temp\\program.exe");

  if (result == 0) {
    system("temp\\program.exe");
    exit(EXIT_SUCCESS);
  } else {
    fprintf(stderr,
            "[DRIVER] Compilation Error: Clang failed to build the IR.\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  const char *filename = argc > 1 ? argv[1] : "main.apl";
  const int length = strlen(filename);

  const char *ext = strrchr(filename, '.');

  if (ext == NULL || strcmp(ext, EXPECTED_EXTENSION) != 0) {
    perror("Expected an .apl file...");
    exit(EXIT_FAILURE);
  }

  char *source = read_file(filename);

  ErrorStack err_stack;

  Lexer lexer;
  lexer.current = source;
  lexer.line = 1;
  lexer.errors = &err_stack;
  lexer.scope_level = 0;

  errorStack_init(&err_stack);

  // Ensure temp directory exists before parsing/codegen
  system("mkdir temp 2> nul");

  // We need the symbol table for parsing, so we init CodegenContext early.
  CodegenContext compiler_context = {0};
  codegen_init(&compiler_context, "temp\\output.bc");

  
  // 1. Parsing Phase
  ASTNode *program_ast = compile_parse(&lexer, &err_stack, &compiler_context);
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }

  // 2. Semantic Analysis Phase
  SemanticContext semantic_ctx = { &err_stack, &compiler_context};

  analyze_semantics(&semantic_ctx, program_ast);


  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }
  // 3. Code Generation Phase
  gen_program_from_ast(&compiler_context, program_ast);
  
  fclose(compiler_context.file);
  free_codegen_context(&compiler_context, &err_stack);

  free(source);
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    exit(EXIT_FAILURE);
  }

  llvm_compilation();

  return 0;
}
