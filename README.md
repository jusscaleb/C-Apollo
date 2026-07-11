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
    functions.h      Function metadata and tracking declarations
    token.h          Token, Lexer, and Parser structures
    variables.h      Symbol table, variable metadata, and codegen declarations
    error.h          Error structures and error stack definitions
  src/
    ast.c            AST node construction helpers
    main.c           Compiler coordinator
    lexer.c          Source text to tokens
    parser.c         Syntax parser and AST builder
    generator.c      LLVM IR generator
    memory.c         Memory management helpers (realloc, calloc, grow symbol list, free context)
    error.c          Error stack handling and formatting functions
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
gcc src/main.c src/lexer.c src/parser.c src/generator.c src/ast.c src/memory.c src/error.c src/variables.c src/semantic.c -o run/main.exe
.\run\main.exe .\main.apl
```

You can replace `.\main.apl` with any `.apl` file path.

## Compiler Pipeline

1. `src/main.c` reads the Apollo source from the provided `.apl` file path.
2. `src/lexer.c` converts source characters into tokens. It catches lexical errors such as unterminated strings.
3. `src/parser.c` validates the Apollo grammar and builds a structured AST for the entire program. It utilizes Panic-Mode Error Recovery (`synchronize()`) to prevent cascading phantom errors when encountering syntax issues.
4. `src/semantic.c` performs a semantic analysis pass over the AST. It enforces type checking, validates variable and function assignments, and handles scope, utilizing the symbol table in `src/variables.c`.
5. If no errors were detected, `src/generator.c` walks the AST and writes optimized LLVM IR to `output.ll`. If errors exist, codegen is safely skipped to avoid compiling a malformed AST.
6. Clang compiles `output.ll` with `-O3` optimization into a native `program.exe`.
7. The generated program runs and prints its output.

## Error Handling

Apollo features a robust, non-halting error detection system:
- **Error Stack**: Errors are pushed onto an internal `errorStack` rather than crashing the compiler. 
- **Panic-Mode Recovery**: When a syntax error occurs, the parser synchronizes itself to the next statement boundary (like a semicolon or major keyword). This prevents a single typo from causing a massive cascade of meaningless subsequent errors.
- **Contextual Formatting**: Errors display their specific type (`Syntax Error`, `Lexical Error`, `Reference Error`), the line number, and a snippet of the exact token that caused the problem `(Found: x)`.

## AST Node Types

Every statement and expression in Apollo passes through an AST node before codegen:

| Node | Purpose |
|---|---|
| `AST_VAR_DECL` | Variable declaration (`var x = ...`) |
| `AST_VAR_ASS` | Variable reassignment (`x = ...`) |
| `AST_PRINTLN` | Print statement (`println(...)`) |
| `AST_LITERAL_EXPR` | A literal value (integer, float, string, bool) |
| `AST_BINARY_EXPR` | A binary arithmetic or logical expression (`left op right`) |
| `AST_VAR_REF` | A reference to a declared variable by name |
| `AST_IF` | Conditional branches (`if`, `elif`, `else`) |
| `AST_WHILE` | Loop structures (`while`) |
| `AST_FOR` | Loop structures (`for` loops) |
| `AST_BLOCK` | Brace-enclosed sequence of statements (`{ ... }`) |
| `AST_CALL_FXN` | Call to a function (`func()`) |

## Requirements

- GCC
- Clang / LLVM
- Windows or a Windows-compatible shell environment such as MSYS2

## Status

Apollo is a small, lightweight compiler compiled with LLVM's `-O3` backend for high performance. It supports:
- Variables of all supported types (integers, floats, booleans, strings, nulls)
- Global variables (accessible across multiple functions)
- Mathematical and logical expressions (`+`, `-`, `*`, `/`, `%`, `and`, `or`, `&&`, `||`)
- String concatenation (`+`), including seamless concatenation with non-string types (integers, floats, booleans)
- Control flow (`if`, `elif`, `else` conditionals)
- Loop structures (`while` and `for` loops)
- Function declarations (`fxn func_name() -> (void) { ... }`)
- Function calls (`func_name();`)
- Printing values and variables via `println`
- **Infinite Function Nesting:** Functions can be deeply nested inside other functions with perfect scoping (ladder-climbing resolution).
- **Name Mangling:** Apollo robustly handles nested function naming collisions by mangling internal LLVM names, allowing you to reuse function names (like `main`) across different scopes safely!

## Testing

Apollo includes an automated integration test suite written in Python. These scripts dynamically create temporary `.apl` test cases, compile them via `apl.exe`, execute the built native binaries, and assert the `stdout` against expected results.

You can run the full test suite using the bash diagnosis script:

```bash
./diagnosis.sh
```

Current limitations:
- Nested block scoping for variables (e.g., variables declared inside `if` statements or `while` loops being fully scoped) is still under development
- No function parameters/arguments yet

The compiler currently successfully isolates function scopes, supports global variables, and effortlessly handles deeply nested functions. The next natural steps are full nested block scoping for variables, parameter support, arrays/structs, and custom command-line options.
