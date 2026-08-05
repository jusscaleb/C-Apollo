# Apollo Programming Language

**Version:** `v3.0.0.WIP-preview`  
**Release Channel:** Active Development / Unstable Preview  
**Version Format:** `MAJOR.MINOR.PATCH.TAG`

Apollo is a compiled programming language featuring a custom compiler frontend and LLVM C-API backend written in C. The pipeline lexes and parses Apollo source code (`.apl`), runs semantic analysis, generates LLVM IR bitcode using the LLVM C API (`libLLVM-19`), links runtime library bitcodes, and compiles down to native Windows executable binaries.

---

## Project Layout

```text
Apollo/
├── apl.c                    Compiler driver wrapper (builds and executes compiler)
├── caleb.apl                Sample Apollo source program
├── headers/
│   ├── arithmetic.h         Arithmetic types and operator declarations
│   ├── ast.h                Abstract Syntax Tree (AST) node structures and constructors
│   ├── defs.h               Codegen context structures and global declarations
│   ├── error.h              Error handling structures and stack declarations
│   ├── functions.h          Function signature and metadata tracking
│   ├── llvm_backend.h       LLVM C API backend function prototypes & LLVMComponents struct
│   ├── semantic.h           Semantic analyzer context and symbol checking prototypes
│   ├── strings.h            String slice and utility declarations
│   ├── token.h              Lexer tokens, Token types, Lexer state & Parser structs
│   └── variables.h          Symbol table entries, variable resolution & scope tracking
├── src/
│   ├── ast.c                AST node allocation and tree construction logic
│   ├── error.c              Error stack management and user-facing error reporting
│   ├── lexer.c              Source code scanner / tokenization engine
│   ├── llvm_backend.c       LLVM IR code generator (LLVM C API: functions, loops, if-else, etc.)
│   ├── main.c               Compiler driver / main entry point
│   ├── memory.c             Arena memory allocation and context cleanup utilities
│   ├── parser.c             Recursive descent parser with panic-mode error recovery
│   ├── semantic.c           Semantic analysis, type inference & scope resolution
│   └── variables.c          Symbol table allocation, lookup, and scope level management
├── run/                     Target directory for compiler binary (`main.exe`)
└── temp/                    Intermediate IR output (`output.bc`) and compiled program (`program.exe`)
```

---

## Quick Start

### 1. Build and Run via `apl.c` Driver

The simplest way to build the compiler and execute an Apollo script:

```powershell
gcc -O2 apl.c -o apl.exe
.\apl.exe caleb.apl
```

This compiles all compiler sources in `src/` against LLVM 19, creates `run/main.exe`, and immediately compiles & runs `caleb.apl`.

### 2. VS Code Task Execution

Open any `.apl` file in VS Code and press:
```text
Ctrl + Shift + B
```
This triggers the pre-configured `.vscode/tasks.json` runner.

### 3. Manual Compiler Build

If building `main.exe` manually using GCC with MSYS2 MinGW-w64:

```powershell
gcc src/main.c src/lexer.c src/parser.c src/ast.c src/memory.c src/error.c src/variables.c src/semantic.c src/llvm_backend.c `
  -IC:\msys64\mingw64\include `
  -LC:\msys64\mingw64\lib `
  -lLLVM-19 `
  -o run/main.exe

.\run\main.exe caleb.apl
```

---

## Compiler Architecture and Pipeline

```mermaid
flowchart LR
    Source[".apl Source"] --> Lexer["Lexer (src/lexer.c)"]
    Lexer --> Parser["Parser (src/parser.c)"]
    Parser --> AST["AST Representation"]
    AST --> Semantic["Semantic Analysis (src/semantic.c)"]
    Semantic --> LLVMBackend["LLVM C-API Backend (src/llvm_backend.c)"]
    LLVMBackend --> Bitcode["LLVM Bitcode (temp/output.bc)"]
    Bitcode --> Clang["Clang Compiler & Linker"]
    Clang --> Executable["Native Binary (program.exe)"]
```

1. **Lexical Analysis (`src/lexer.c`)**: Converts character streams into structured `Token` streams. Catches invalid characters and unterminated strings.
2. **Syntactic Analysis (`src/parser.c`)**: Constructs an Abstract Syntax Tree (AST) using recursive descent parsing. Features **Panic-Mode Error Recovery** (`synchronize()`) to catch syntax errors without crashing or generating cascading false positives.
3. **Semantic Analysis (`src/semantic.c`)**: 
   - Resolves symbol references and scope levels.
   - Performs strict type checking on assignments, variable declarations, and returns.
   - Infers types for arithmetic (`+`, `-`, `*`, `/`) and logical comparison operations (`>`, `<`, `>=`, `<=`, `==`, `!=`, `and`, `or`), correctly evaluating comparison and logical ops as `TYPE_BOOL`.
4. **LLVM Code Generation (`src/llvm_backend.c`)**:
   - Emits LLVM IR using the official LLVM C API (`libLLVM-19`).
   - Manages basic blocks for function entry/exit, conditionals (`if`/`else`), `while` loops, and `for` loops.
   - Handles variable allocation (`alloca`), loads (`load`), stores (`store`), and expression evaluation (`arihmetics()`).
   - Integrates `x++`, `x--`, `+=`, `-=`, `*=`, `/=`, and `%=` desugaring into LLVM reassignment logic.
   - Validates generated LLVM modules via `LLVMVerifyModule`.
   - Writes generated IR to bitcode file `temp/output.bc`.
5. **Native Code Compilation**: Invokes Clang to compile `temp/output.bc` alongside Apollo's built-in runtime library into native executable binaries (`program.exe`).

---

## Language Features and Syntax Overview

### Function Declarations and Execution Entry

```apl
fxn run() -> (void) {
  println("Hello from Apollo!");
}
```

- Functions use the `fxn` keyword.
- `run()` serves as the main entry point (compiled to `main` in LLVM IR).

### Variable Declarations and Reassignments

```apl
var x = 10;
#var word = false;

x = x + 5;
x++;
x--;
```

- Supports types: `int`, `float`, `bool`, `str`, `null`.
- Explicit syntax for declarations (`var` / `#var` / `#int`, `#float`, `#bool`, `#str`).
- In-place increments (`++`) and decrements (`--`) desugar cleanly to assignments.

### Control Flow

#### `if` / `else` Statements
```apl
if (x > 5) {
  println("x is greater than 5");
} else {
  println("x is 5 or less");
}
```

#### `while` Loops
```apl
var count = 0;
while (count < 5) {
  println("Count: ", count);
  count++;
}
```

#### `for` Loops
```apl
for (var i = 0; i < 5; i++) {
  println("Value ", i);
}
```

### Printing Outputs (`println`)

```apl
println("Result: ", 45 > 3);
```

Supports variable arguments of multiple types (strings, integers, floats, booleans) printed in sequence.

---

## LLVM Bitcode Inspection

To inspect the generated LLVM assembly IR (`output.ll`) from bitcode (`output.bc`), ensure `llvm-dis` is on your PATH (e.g., `C:\msys64\mingw64\bin`) and run:

```powershell
llvm-dis temp\output.bc -o temp\output.ll
Get-Content temp\output.ll
```

---

## Requirements

- **GCC**: MinGW-w64 (MSYS2 recommended)
- **LLVM 19**: Header files and `libLLVM-19` dynamic library
- **Clang**: Native linker/compiler toolchain for Windows

---

## License

Apollo is released under the **MIT License**. Feel free to modify, build upon, and distribute under its terms.
