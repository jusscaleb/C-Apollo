#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <process.h>
#include <string.h>

int main(int argc, char **argv) {
  char project_root_buf[512];
  const char *project_root = argc > 2 ? argv[2] : getcwd(project_root_buf, sizeof(project_root_buf));
  if (project_root == NULL) {
    perror("Could not resolve project root");
    return 1;
  }

  // Make sure the generated compiler can find LLVM's runtime DLLs.
  {
    const char *llvm_bin = "C:\\msys64\\mingw64\\bin";
    char path_buf[2048];
    const char *old_path = getenv("PATH");

    if (old_path != NULL && old_path[0] != '\0') {
      snprintf(path_buf, sizeof(path_buf), "%s;%s", llvm_bin, old_path);
    } else {
      snprintf(path_buf, sizeof(path_buf), "%s", llvm_bin);
    }

    _putenv_s("PATH", path_buf);
  }

  char mkdir_command[512];
  snprintf(mkdir_command, sizeof(mkdir_command), "mkdir \"%s\\run\" 2>nul",
           project_root);
  system(mkdir_command);

  char build_command[2048];
  printf("Beginning build.\n");
  snprintf(build_command, sizeof(build_command),
           "gcc -v \"-IC:\\msys64\\mingw64\\include\" \"%s\\src\\main.c\" \"%s\\src\\lexer.c\" \"%s\\src\\parser.c\" "
           "\"%s\\src\\ast.c\" \"%s\\src\\memory.c\" "
           "\"%s\\src\\error.c\" \"%s\\src\\variables.c\" \"%s\\src\\semantic.c\" "
           "\"%s\\src\\API\\llvm_main.c\" \"%s\\src\\API\\llvm_fxns.c\" \"%s\\src\\API\\llvm_variables.c\" "
           "\"%s\\src\\API\\llvm_helpers.c\" \"%s\\src\\API\\llvm_condbr.c\" "
           "-LC:\\msys64\\mingw64\\lib -lLLVM-19 -o \"%s\\run\\main.exe\"",
           project_root, project_root, project_root, project_root,
           project_root, project_root, project_root, project_root,
           project_root, project_root, project_root, project_root, project_root, project_root);

  int run = system(build_command);

  printf("Build finished with exit code: %d\n", run);
  if (run != 0) {
    perror("Could not build program");
    return 1;
  }

  if (argc > 1) {
    char main_exe[512];
    snprintf(main_exe, sizeof(main_exe), "%s\\run\\main.exe", project_root);
    return _spawnl(_P_WAIT, main_exe, main_exe, argv[1], NULL);
  }

  char main_exe[512];
  snprintf(main_exe, sizeof(main_exe), "%s\\run\\main.exe", project_root);
  return _spawnl(_P_WAIT, main_exe, main_exe, NULL);
}
