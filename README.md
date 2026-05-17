# Apollo Programming Language

Apollo is a small experimental programming language and compiler written in C. The current compiler pipeline lexes and parses a minimal Apollo program, generates LLVM IR, and uses Clang to build a native Windows executable.

## Current Language Shape

Apollo currently supports a single entrypoint function:

```apollo
fxn run() -> (void){
   println("Hello \"There\" Caleb");
   println("How are you today");
}
```

Supported syntax at this stage:

- `fxn` function declarations
- `run` as the program entrypoint
- `-> (void)` return signature
- block bodies with `{ ... }`
- `println("...");` string output statements
- displayable string literals, including escaped quotes, newlines, tabs, and backslashes

## Project Layout

```text
Apollo/
  apl.c              Compiler driver helper
  headers/
    defs.h           Shared definitions and codegen declarations
    token.h          Token, lexer, and parser structures
  src/
    main.c           Compiler coordinator
    lexer.c          Source text to tokens
    parser.c         Syntax parser and codegen handoff
    generator.c      LLVM IR generator
  .vscode/
    tasks.json       VS Code task for running the active Apollo file
  run/               Built executable output folder
```

## Run An Apollo File

Apollo files use the `.apl` extension. A valid Apollo file currently looks like this:

```apollo
fxn run() -> (void){
   println("Hello from Apollo");
}
```

In VS Code, open any `.apl` file and press:

```text
Ctrl + Shift + B
```

The configured task builds `apl.c`, moves into the active file's directory, and runs that file through the Apollo compiler.

The task effectively does:

```powershell
gcc apl.c -o apl.exe
cd <active-file-directory>
apl.exe <active-file-path> <project-root>
```

## Manual Build

The driver compiles the compiler sources with GCC and then runs the selected `.apl` file:

```powershell
gcc src/main.c src/lexer.c src/parser.c src/generator.c -o run/main.exe
.\run\main.exe .\main.apl
```

You can replace `.\main.apl` with any `.apl` file path.

## Compiler Pipeline

1. `src/main.c` reads Apollo source from the provided `.apl` file path.
2. `src/lexer.c` converts source characters into tokens.
3. `src/parser.c` validates the expected Apollo grammar.
4. `src/generator.c` writes LLVM IR to `output.ll`, including LLVM-safe string output.
5. Clang compiles `output.ll` into `program.exe`.
6. The generated program runs and prints its output.

## Requirements

- GCC
- Clang / LLVM
- Windows or a Windows-compatible shell environment such as MSYS2

## Status

Apollo is early-stage and intentionally small. It can now read source files and display string literals through `println`. The next natural steps are expanding statement parsing, adding more types, improving command-line options, and separating global string constants from function body generation.
