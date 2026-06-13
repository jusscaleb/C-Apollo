# Apollo Programming Language

Apollo is a small programming language and compiler written in C. The compiler pipeline lexes and parses an Apollo program, generates LLVM IR, and uses Clang to build a native Windows executable.

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
    parser.c         Syntax parser and AST builder
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
3. `src/parser.c` validates the Apollo grammar and builds AST nodes for all statements.
4. `src/generator.c` walks the AST and writes LLVM IR to `output.ll`, covering string/integer/decimal/boolean/arithmetic output, variable allocation, variable references in expressions, variable reassignment (including dynamic strings), and variable printing.
5. Clang compiles `output.ll` into `program.exe`.
6. The generated program runs and prints its output.

## AST Node Types

Every statement and expression in Apollo passes through an AST node before codegen:

| Node | Purpose |
|---|---|
| `AST_VAR_DECL` | Variable declaration (`var x = ...`) |
| `AST_VAR_ASS` | Variable reassignment (`x = ...`) |
| `AST_PRINTLN` | Print statement (`println(...)`) |
| `AST_LITERAL_EXPR` | A literal value (integer, float, string, bool) |
| `AST_BINARY_EXPR` | A binary arithmetic expression (`left op right`) |
| `AST_VAR_REF` | A reference to a declared variable by name |

## Requirements

- GCC
- Clang / LLVM
- Windows or a Windows-compatible shell environment such as MSYS2

## Status

Apollo is early-stage and intentionally small. It can read source files, skip `#` line comments, declare and store variables of all supported types, reassign variables (including dynamically resizing strings), reference variables inside expressions, and print both literal values and declared variables through `println`.

Current limitations:

- only one entrypoint function `run` is supported
- no control flow (if, loops) yet

The next natural steps are control flow, multiple functions, and improving command-line options.
