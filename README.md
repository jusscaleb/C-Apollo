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
  output/
    main.apl         Default Apollo source file
  run/               Built executable output folder
```

## Build

From the `output` folder, build and run the compiler driver:

```powershell
cd output
.\apl.exe
```

The driver compiles the compiler sources with GCC:

```powershell
gcc ../src/main.c ../src/lexer.c ../src/parser.c ../src/generator.c -o ../run/main.exe
```

Then it runs:

```powershell
..\run\main.exe
```

By default, the compiler reads:

```text
output/main.apl
```

You can pass another Apollo source file path to the compiler executable:

```powershell
..\run\main.exe path\to\file.apl
```

## Compiler Pipeline

1. `src/main.c` reads Apollo source from `main.apl` or a provided file path.
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
