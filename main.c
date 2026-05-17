/*Coordinator*/

#include <stdio.h>
#include "headers/token.h"

// Link our global parser orchestration entrypoint
void compile_parse(Lexer* lexer);

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

    return 0;
}