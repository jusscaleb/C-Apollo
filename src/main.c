/*Coordinator*/

#include <direct.h>
#include "../headers/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../headers/error.h"
#include "../headers/ast.h"
#include "../headers/semantic.h"
#include "../headers/defs.h"
#include "../headers/llvm_backend.h"

ASTNode *compile_parse(Lexer *lexer, ErrorStack *s, CodegenContext *context);

static FILE *trace_file = NULL;

static void trace(const char *message) {
  if (!trace_file) {
    trace_file = fopen("trace.log", "a");
  }
  if (trace_file) {
    fprintf(trace_file, "%s\n", message);
    fflush(trace_file);
  }
}

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
  char cwd[512];
  if (_getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("Could not resolve current working directory");
    exit(EXIT_FAILURE);
  }

  char bc_path[1024];
  char exe_path[1024];
  snprintf(bc_path, sizeof(bc_path), "\"%s\\temp\\output.bc\"", cwd);
  snprintf(exe_path, sizeof(exe_path), "\"%s\\temp\\program.exe\"", cwd);

  char command[2048];
  snprintf(command, sizeof(command), "clang -O3 %s -o %s", bc_path, exe_path);
  int result = system(command);

  if (result == 0) {
    printf("--- Running Apollo Program Output ---\n");
    system(exe_path);
    printf("-------------------------------------\n");
    exit(EXIT_SUCCESS);
  } else {
    fprintf(stderr,
            "[DRIVER] Compilation Error: Clang failed to build the IR.\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  trace("entered main");
  const char *filename = argc > 1 ? argv[1] : "main.apl";

  const char *ext = strrchr(filename, '.');

  if (ext == NULL || strcmp(ext, EXPECTED_EXTENSION) != 0) {
    trace("bad extension");
    perror("Expected an .apl file...");
    exit(EXIT_FAILURE);
  }

  trace("reading source");
  char *source = read_file(filename);

  ErrorStack err_stack;

  Lexer lexer;
  lexer.current = source;
  lexer.line = 1;
  lexer.errors = &err_stack;
  lexer.scope_level = 0;

  errorStack_init(&err_stack);
  trace("error stack initialized");

  // Ensure temp directory exists before parsing/codegen
  fprintf(stderr, "[DRIVER] Ensuring temp directory exists.\n");
  fflush(stderr);
  trace("ensuring temp directory");
  {
    char cwd[512];
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("Could not resolve current working directory");
      free(source);
      exit(EXIT_FAILURE);
    }

    char mkdir_command[1024];
    snprintf(mkdir_command, sizeof(mkdir_command), "mkdir \"%s\\temp\" 2>nul",
             cwd);
    system(mkdir_command);
  }

  // We need the symbol table for parsing, so we init CodegenContext early.
  fprintf(stderr, "[DRIVER] Initializing codegen context.\n");
  fflush(stderr);
  trace("initializing codegen context");
  CodegenContext compiler_context = {0};
  {
    char cwd[512];
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
      perror("Could not resolve current working directory");
      free(source);
      exit(EXIT_FAILURE);
    }

    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%s\\temp\\output.bc", cwd);
    codegen_init(&compiler_context, output_path);
  }
  fprintf(stderr, "[DRIVER] Codegen context initialized.\n");
  fflush(stderr);

  fprintf(stderr, "[DRIVER] Starting parse for %s\n", filename);
  fflush(stderr);
  trace("starting parse");
  // 1. Parsing Phase
  ASTNode *program_ast = compile_parse(&lexer, &err_stack, &compiler_context);

  fprintf(stderr, "[DRIVER] Parsing finished.\n");
  fflush(stderr);
  trace("parsing finished");
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }

  // 2. Semantic Analysis Phase
  SemanticContext semantic_ctx = { &err_stack, &compiler_context};

  fprintf(stderr, "[DRIVER] Starting semantic analysis.\n");
  fflush(stderr);
  trace("starting semantic analysis");
  analyze_semantics(&semantic_ctx, program_ast);
  fprintf(stderr, "[DRIVER] Semantic analysis finished.\n");
  fflush(stderr);
  trace("semantic analysis finished");
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }

  // 3. Code Generation Phase
  fprintf(stderr, "[DRIVER] Starting LLVM backend emission.\n");
  fflush(stderr);
  trace("starting llvm backend emission");
  LLVMComponents components = {0};
  
  _apl_llvm_environment_setup(&compiler_context, &components, program_ast);
  fprintf(stderr, "[DRIVER] LLVM backend emission finished.\n");
  fflush(stderr);
  trace("llvm backend emission finished");
  

  free_codegen_context(&compiler_context, &err_stack);

  free(source);

  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "[DRIVER] Starting clang compilation.\n");
  fflush(stderr);
  trace("starting clang compilation");
  llvm_compilation();

  return 0;
}
