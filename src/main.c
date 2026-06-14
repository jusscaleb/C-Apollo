/*Coordinator*/

#include "../headers/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Link our global parser orchestration entrypoint
void compile_parse(Lexer *lexer);

static char *read_file(const char *filename) {
  FILE *file = fopen(filename, "rb");

  if (file == NULL) {
    fprintf(stderr, "[DRIVER] Could not open source file: %s\n", filename);
    exit(0);
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char *source = malloc(file_size + 1);
  if (source == NULL) {
    fprintf(stderr, "[DRIVER] Could not allocate memory for source file.\n");
    fclose(file);
    exit(0);
  }

  size_t bytes_read = fread(source, 1, file_size, file);
  source[bytes_read] = '\0';

  fclose(file);
  return source;
}

// Automatically runs the program after successfully compilation.
void llvm_compilation() {
  int result = system("clang -O3 output.ll -o program.exe");

  if (result == 0) {
    printf("--- Running Apollo Program Output ---\n");
    system(".\\program.exe");
    printf("-------------------------------------\n");
    exit(0);
  } else {
    fprintf(stderr,
            "[DRIVER] Compilation Error: Clang failed to build the IR.\n");
    exit(0);
  }
}

int main(int argc, char **argv) {
  const char *filename = argc > 1 ? argv[1] : "main.apl";
  const int length = strlen(filename);

  const char *ext = strrchr(filename, '.');

  if (ext == NULL || strcmp(ext, EXPECTED_EXTENSION) != 0) {
    perror("Expected an .apl file...");
    exit(0);
  }

  char *source = read_file(filename);

  Lexer lexer;
  lexer.current = source;
  lexer.line = 1;

  // Pass the raw lexer configuration over to the syntax parser engine
  compile_parse(&lexer);
  free(source);

  llvm_compilation();

  return 0;
}
