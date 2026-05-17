/*Coordinator*/

#include <stdio.h>
#include <stdlib.h>
#include "../headers/token.h"

// Link our global parser orchestration entrypoint
void compile_parse(Lexer* lexer);


//Automatically runs the program after successfully compilation.
void llvm_compilation(){
    int result = system("clang output.ll -o program.exe");
    
    if (result == 0) {
        printf("--- Running Apollo Program Output ---\n");
        system(".\\program.exe");
        printf("-------------------------------------\n");
        exit(0);
    } else {
        fprintf(stderr, "[DRIVER] Compilation Error: Clang failed to build the IR.\n");
        exit(1);
    }
}

int main() {
    // Target source code block setup matching your syntax structure
    const char* source = 
        "fxn run() -> (void){"
        "   println(\"hello world.\");\n"
        "}";

    Lexer lexer;
    lexer.current = source;
    lexer.line = 1;

    printf("Initiating Apollo Compilation Pipeline...\n");
    printf("-------------------------------------------\n");

    // Pass the raw lexer configuration over to the syntax parser engine
    compile_parse(&lexer);

    llvm_compilation();

    return 0;
}
