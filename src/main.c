#include <direct.h>
#include "../headers/token.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../headers/error.h"
#include "../headers/ast.h"
#include "../headers/semantic.h"
#include "../headers/defs.h"
#include "../headers/llvm_backend.h"
#include "../headers/memory.h"

#include <process.h>


ASTNode *compile_parse(Lexer *lexer, ErrorStack *s, CodegenContext *context);

static char *path_from_cwd(const char *suffix) {
  char *cwd = _getcwd(NULL, 0);
  if (cwd == NULL) {
    return NULL;
  }

  size_t cwd_len = strlen(cwd);
  size_t suffix_len = strlen(suffix);
  if (suffix_len > (size_t)-1 - cwd_len - 1) {
    free(cwd);
    errno = ENOMEM;
    return NULL;
  }

  char *path = (char *)malloc(cwd_len + suffix_len + 1);
  if (path == NULL) {
    free(cwd);
    return NULL;
  }

  memcpy(path, cwd, cwd_len);
  memcpy(path + cwd_len, suffix, suffix_len + 1);
  free(cwd);
  return path;
}

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

// Automatically runs the program after successful compilation.
void llvm_compilation() {
  char *obj_path = path_from_cwd("\\temp\\output.o");
  char *exe_path = path_from_cwd("\\temp\\program.exe");
  if (obj_path == NULL || exe_path == NULL) {
    perror("Could not resolve current working directory");
    free(obj_path);
    free(exe_path);
    exit(EXIT_FAILURE);
  }

  // Link the native object file directly — no bitcode re-parsing needed.
  const char *clang_bin = "C:\\msys64\\clang64\\bin\\clang.exe";
  int result = _spawnl(_P_WAIT, clang_bin, "clang", "--target=x86_64-w64-windows-gnu", "-fuse-ld=lld", obj_path, "-o", exe_path, NULL);
  if (result != 0) {
    result = _spawnlp(_P_WAIT, "clang", "clang", "--target=x86_64-w64-windows-gnu", "-fuse-ld=lld", obj_path, "-o", exe_path, NULL);
  }

  free(obj_path);

  if (result == 0) {
    _spawnl(_P_WAIT, exe_path, exe_path, NULL);
    free(exe_path);

    exit(EXIT_SUCCESS);
  } else {
    free(exe_path);
    fprintf(stderr,
            "[DRIVER] Compilation Error: Linker failed to build executable.\n");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  
  if(_DB) trace("entered main");
  const char *filename = argc > 1 ? argv[1] : "main.apl";

  const char *ext = strrchr(filename, '.');

  if (ext == NULL || memcmp(ext, EXPECTED_EXTENSION, 3) != 0) {
    if(_DB) trace("bad extension");
    perror("Expected an .apl file...");
    exit(EXIT_FAILURE);
  }

  if(_DB) trace("reading source");
  char *source = read_file(filename);

  Arena arena;

  SymbolTable table;

  arena_init(1024*1024, &arena);
  ErrorStack err_stack;

  Lexer lexer;
  lexer.current = source;
  lexer.line = 1;
  lexer.column = 0;
  lexer.errors = &err_stack;
  lexer.scope_level = 0;

  errorStack_init(&err_stack);
  if(_DB) trace("error stack initialized");

  // Ensure temp directory exists natively without shell overhead
  _DEBUG("[DRIVER] Ensuring temp directory exists.")
  fflush(stderr);
  if(_DB) trace("ensuring temp directory");
  _mkdir("temp");

  // We need the symbol table for parsing, so we init CodegenContext early.
  _DEBUG("[DRIVER] Initializing codegen context.");
  fflush(stderr);
  if(_DB) trace("initializing codegen context");
  CodegenContext compiler_context = {0};

  compiler_context.a = &arena;


  compiler_context.t = symbol_table_init(compiler_context.a, 256);
  {
    char *output_path = path_from_cwd("\\temp\\output.bc");
    if (output_path == NULL) {
      perror("Could not resolve current working directory");
      free(source);
      exit(EXIT_FAILURE);
    }
    codegen_init(&compiler_context, output_path);
    free(output_path);
  }
  _DEBUG("[DRIVER] Codegen context initialized.");
  fflush(stderr);

  _DEBUG("[DRIVER] Starting parse.");
  fflush(stderr);
  if(_DB) trace("starting parse");
  // 1. Parsing Phase
  ASTNode *program_ast = compile_parse(&lexer, &err_stack, &compiler_context);

  _DEBUG("[DRIVER] Parsing finished.");

  fflush(stderr);
  if(_DB) trace("parsing finished");
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }

  // 2. Semantic Analysis Phase
  SemanticContext semantic_ctx = { &err_stack, &compiler_context};

_DEBUG("[DRIVER] Starting semantic analysis.");

  fflush(stderr);
  if(_DB) trace("starting semantic analysis");
  analyze_semantics(&semantic_ctx, program_ast);
  _DEBUG("[DRIVER] Semantic analysis finished.");
  fflush(stderr);
  if(_DB) trace("semantic analysis finished");
  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    free(source);
    exit(EXIT_FAILURE);
  }

  // 3. Code Generation Phase
  _DEBUG("[DRIVER] Starting LLVM backend emission.");
  fflush(stderr);
  if(_DB) trace("starting llvm backend emission");
  LLVMComponents components = {0};
  
  _apl_llvm_environment_setup(&components, program_ast);

  _DEBUG("[DRIVER] LLVM backend emission finished.");
  fflush(stderr);
  if(_DB) trace("llvm backend emission finished");
  
  arena_reset(compiler_context.a);
  arena_free(compiler_context.a);

  free(source);

  if (err_stack.size > 0) {
    errorStack_seek(&err_stack);
    errorStack_free(&err_stack);
    exit(EXIT_FAILURE);
  }

   _DEBUG("[DRIVER] Starting clang compilation.");

  fflush(stderr);
  if(_DB) trace("starting clang compilation");
  llvm_compilation();

  return 0;
}
