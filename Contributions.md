# Contributing to Apollo

Thank you for your interest in contributing to the Apollo Programming Language.

Apollo is a modular, compiled programming language built in C99 featuring a custom frontend (lexer, modular Pratt parser, AST, semantic analyzer), an LLVM C-API backend (`libLLVM-19`), and pre-compiled runtime bitcode modules (`apollo-modules`).

Whether you are fixing bugs, improving documentation, optimizing LLVM IR generation, or proposing new syntax and language features, your contributions are welcome.

---

## Prerequisites and Local Environment Setup

To build and work on the Apollo compiler codebase, ensure you have the following installed:

1. **C Compiler**: GCC (MinGW-w64 on Windows via MSYS2 recommended).
2. **Build System**: CMake (v3.20 or newer).
3. **LLVM Toolchain**: LLVM 19 development libraries (`libLLVM-19`), headers, and `clang`.
4. **LLVM Utilities (Optional but Recommended)**: `llvm-dis` for disassembling generated bitcode (`.bc`) into human-readable IR (`.ll`).

### Adding LLVM to your PATH (Windows / MSYS2)

Ensure MSYS2's MinGW bin path is added to your System PATH environment variable:
```powershell
# Example path for MSYS2 MinGW-w64
C:\msys64\mingw64\bin
```

---

## Building and Testing Locally

### 1. Fast Build & Run with `run.sh`

The `run.sh` script configures CMake, compiles all C source files across `src/`, `src/API/`, `src/Parser/`, and `src/Semantics/`, and executes your target `.apl` file:

```bash
# Build compiler and execute target script
./run.sh caleb.apl

# Run existing build without re-configuring CMake
./run.sh build caleb.apl
```

### 2. Manual CMake Build

```powershell
# Generate build directory
cmake -B build -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Compile executable
cmake --build build

# Execute an Apollo script
./build/apollo.exe caleb.apl
```

### 3. Direct GCC Compiler Build

```powershell
gcc src/main.c src/ast.c src/error.c src/lexer.c src/memory.c src/variables.c `
    src/API/*.c src/Parser/*.c src/Semantics/*.c `
    -Isrc -Isrc/API -Isrc/Parser -Isrc/Semantics -Iheaders `
    -IC:\msys64\mingw64\include `
    -LC:\msys64\mingw64\lib `
    -lLLVM-19 -o build/apollo.exe

./build/apollo.exe caleb.apl
```

### 4. Running the Automated Test Suite

Apollo uses a Python test runner located in `tests/`:

```powershell
# Run the automated test suite
python tests/test.py

# Run diagnostic script
./diagnosis.sh
```

---

## Codebase Architecture

Understanding the compiler stages will help you locate where to make changes:

| Component | Directory / Files | Description |
|---|---|---|
| **Lexer** | `src/lexer.c`, `headers/token.h` | Scans raw `.apl` source text into a stream of typed tokens. |
| **Parser** | `src/Parser/*`, `headers/parser.h` | Modular recursive descent parser with Pratt expression parsing (`parser_expr.c`) and panic recovery (`parser_helpers.c`). |
| **AST System** | `src/ast.c`, `headers/ast.h` | Defines AST node structures, enums, constructors, and tree allocation logic. |
| **Semantic Analysis** | `src/Semantics/*`, `headers/semantic.h` | Performs static type checking, type inference, symbol table registration, and scope level checks. |
| **LLVM Backend** | `src/API/*`, `headers/llvm_backend.h` | Generates LLVM IR bitcode using LLVM C-API (`llvm_main.c`, `llvm_fxns.c`, `llvm_condbr.c`, `llvm_variables.c`). |
| **Runtime Modules** | `apollo-modules/*` | Pre-compiled runtime bitcodes (`apl-io`, `apl-string`, `apl-sys`) linked into target modules during codegen. |
| **Symbol & Memory** | `src/variables.c`, `src/memory.c` | Manages symbol tables, scope ladders, and arena allocations (`Arena`). |
| **Error Handling** | `src/error.c`, `headers/error.h` | Formats and tracks syntax and semantic diagnostic error stacks. |

---

## Coding and Design Guidelines

1. **C99 Standard Compatibility**: Write clean, portable C code.
2. **LLVM Basic Block Termination**: When working on control flow in `src/API/llvm_condbr.c` (`if`, `while`, `for`), every LLVM basic block **must** end with a valid terminator instruction (`LLVMBuildBr`, `LLVMBuildCondBr`, or `LLVMBuildRet`).
3. **No Silent Error Suppression**: Always use the built-in error reporting stack (`report_semantic_error()`, `error()`) rather than silently failing.
4. **AST Safety**: Ensure union field accesses on `ASTNode` match `node->Type`.
5. **Memory Management**: Allocate compiler nodes through `arena_alloc()` to ensure proper lifetime management and $O(1)$ cleanup.

---

## How to Submit a Contribution

1. **Fork the Repository** and create a feature branch (`git checkout -b feature/my-new-feature`).
2. **Commit your changes** with clear, descriptive commit messages.
3. **Verify compilation & test suite**: Ensure `cmake --build build` compiles cleanly and `python tests/test.py` passes all test cases.
4. **Push to your branch** (`git push origin feature/my-new-feature`).
5. **Open a Pull Request** explaining what changes were made and why.

Thank you for helping build Apollo!

