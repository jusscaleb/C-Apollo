#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  const char *project_root = argc > 2 ? argv[2] : ".";

  char mkdir_command[512];
  snprintf(mkdir_command, sizeof(mkdir_command), "mkdir \"%s\\run\" 2>nul",
           project_root);
  system(mkdir_command);

  char build_command[2048];
  snprintf(build_command, sizeof(build_command),
           "gcc \"%s\\src\\main.c\" \"%s\\src\\lexer.c\" \"%s\\src\\parser.c\" "
           "\"%s\\src\\ast.c\" \"%s\\src\\memory.c\" "
           "\"%s\\src\\error.c\" \"%s\\src\\variables.c\" \"%s\\src\\semantic.c\" "
           "\"%s\\src\\llvm_backend.c\" -IC:\\msys64\\mingw64\\include "
           "-LC:\\msys64\\mingw64\\lib -lLLVM-19 -o \"%s\\run\\main.exe\"",
           project_root, project_root, project_root, project_root,
           project_root, project_root, project_root, project_root,
           project_root, project_root);

  int run = system(build_command);

  if (run != 0) {
    perror("Could not build program");
    return 1;
  }

  if (argc > 1) {
    char command[512];
    snprintf(command, sizeof(command), "call \"%s\\run\\main.exe\" \"%s\"",
             project_root, argv[1]);
    return system(command);
  }

  char command[512];
  snprintf(command, sizeof(command), "call \"%s\\run\\main.exe\"",
           project_root);
  return system(command);
}
