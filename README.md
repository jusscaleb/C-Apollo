# Apollo Programming Language

Apollo is a small programming language and compiler written in C. The compiler pipeline lexes and parses an Apollo program, generates LLVM IR, and uses Clang to build a native Windows executable.

## Current Language Shape

Supported syntax at this stage:
- `fxn` function declarations
- `run` as the program entrypoint
- `-> (void)` return signature
- block bodies with `{ ... }`
- `#` line comments
- `var age = 45;` integer variable declarations
- `var name = "Apollo";` string variable declarations
- `var score = 5.5;` decimal variable declarations
- `var ready = true;` boolean variable declarations
- `var total = 5 + 2 * 3;` arithmetic variable declarations
- `println("...");` string literal output
- `println(123);` integer literal output
- `println(5.5);` decimal literal output
- `println(5 + 5);` arithmetic expression output
- `println(age);` integer variable output
- `println(name);` string variable output
- chained arithmetic expressions, such as `println(1 + 2 + 3 + 4);`
- operator precedence for `*`, `/`, and `%` before `+` and `-`
- subtraction, multiplication, division, modulus
- displayable string literals, including escaped quotes, newlines, tabs, and backslashes
- unsigned integer literals
- unsigned decimal literals, emitted as LLVM `double` values
- mixed decimal arithmetic converts the integer side to LLVM `double`
- decimal modulus emitted with LLVM `frem`

## Project Layout

```text
Apollo/
  apl.c              Compiler driver helper
  headers/
    arithmetic.h     Arithmetic expression helpers and declarations
    ast.h            AST node structures and codegen bridge declarations
    defs.h           CodegenContext struct and shared codegen declarations
    token.h          Token, Lexer, and Parser structures
    variables.h      Symbol table, variable metadata, and codegen declarations
  src/
    ast.c            AST node construction helpers
    main.c           Compiler coordinator
    lexer.c          Source text to tokens
    parser.c         Syntax parser, AST builder, and symbol registration
    generator.c      LLVM IR generator
  .vscode/
    tasks.json       VS Code task for running the active Apollo file
  run/               Built executable output folder
```

## Run An Apollo File

Apollo files use the `.apl` extension. In VS Code, open any `.apl` file and press:

```text
Ctrl + Shift + B
```

The configured task builds `apl.c`, moves into the active file's directory, and runs it through the Apollo compiler.

The task effectively does:

```powershell
gcc apl.c -o apl.exe
cd <active-file-directory>
apl.exe <active-file-path> <project-root>
```

## Manual Build

```powershell
gcc src/main.c src/lexer.c src/parser.c src/generator.c src/ast.c -o run/main.exe
.\run\main.exe .\main.apl
```

You can replace `.\main.apl` with any `.apl` file path.

## Compiler Pipeline

1. `src/main.c` reads the Apollo source from the provided `.apl` file path.
2. `src/lexer.c` converts source characters into tokens.
3. `src/parser.c` validates the Apollo grammar, builds AST nodes, and registers declared variables into the `CodegenContext` symbol table.
4. `src/generator.c` writes LLVM IR to `output.ll`, covering string/integer/decimal/boolean/arithmetic output, variable allocation, and variable printing.
5. Clang compiles `output.ll` into `program.exe`.
6. The generated program runs and prints its output.

## Requirements

- GCC
- Clang / LLVM
- Windows or a Windows-compatible shell environment such as MSYS2

## Status

Apollo is early-stage and intentionally small. It can now read source files, skip `#` line comments, declare and store variables of all supported types, and print both literal values and declared variables through `println`.

Current limitations:

- expression operands are currently literal values only (no variable references in expressions)
- only one entrypoint function `run` is supported
- no control flow (if, loops) yet

The next natural steps are adding variable references inside expressions, control flow, multiple functions, and improving command-line options.
