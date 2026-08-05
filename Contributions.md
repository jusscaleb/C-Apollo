# Contributing to Apollo

Thank you for your interest in contributing to the Apollo Programming Language.

Apollo is a lightweight, compiled programming language built in C featuring a custom frontend (lexer, parser, AST, semantic analyzer) and an LLVM C-API backend (`libLLVM-19`).

Whether you are fixing bugs, improving documentation, optimizing LLVM IR generation, or proposing new syntax and language features, your contributions are welcome.

---

## Prerequisites and Local Environment Setup

To build and work on the Apollo compiler codebase, ensure you have the following installed:

1. **C Compiler**: GCC (MinGW-w64 on Windows via MSYS2 recommended).
2. **LLVM Toolchain**: LLVM 19 development libraries (`libLLVM-19`), headers, and `clang`.
3. **LLVM Utilities (Optional but Recommended)**: `llvm-dis` for disassembling generated bitcode (`.bc`) into human-readable IR (`.ll`).

### Adding LLVM to your PATH (Windows / MSYS2)

Ensure MSYS2's MinGW bin path is added to your Environment PATH:
```powershell
# Example path for MSYS2 MinGW-w64
C:\msys64\mingw64\bin
```

---

## Building and Testing Locally

### 1. Fast Build & Run with `apl.c`

The `apl.c` script automatically compiles all C source files in `src/` against LLVM 19 and runs your target `.apl` file:

```powershell
gcc -O2 apl.c -o apl.exe
.\apl.exe caleb.apl
```

### 2. Manual Compiler Build

```powershell
gcc src/main.c src/lexer.c src/parser.c src/ast.c src/memory.c src/error.c src/variables.c src/semantic.c src/llvm_backend.c `
  -IC:\msys64\mingw64\include `
  -LC:\msys64\mingw64\lib `
  -lLLVM-19 `
  -o run/main.exe

.\run\main.exe <path-to-script>.apl
```

### 3. Running Diagnostic & Integration Tests

Run the diagnostic test runner script:
```bash
./diagnosis.sh
```

---

## Codebase Architecture

Understanding the compiler stages will help you locate where to make changes:

| Component | Files | Description |
|---|---|---|
| **Lexer** | `src/lexer.c`, `headers/token.h` | Scans raw `.apl` source text into a stream of tokens. |
| **Parser** | `src/parser.c`, `headers/token.h` | Builds the AST via recursive descent and implements panic-mode recovery (`synchronize()`). |
| **AST System** | `src/ast.c`, `headers/ast.h` | Defines AST node structures, enums, and constructors. |
| **Semantic Analysis** | `src/semantic.c`, `headers/semantic.h` | Type checking, type inference (`TYPE_BOOL`, `TYPE_INT`, etc.), and scope symbol resolution. |
| **LLVM Backend** | `src/llvm_backend.c`, `headers/llvm_backend.h` | Generates LLVM IR bitcode using LLVM C-API functions (`LLVMBuildCondBr`, `LLVMBuildBr`, `arihmetics()`). |
| **Symbol & Memory** | `src/variables.c`, `src/memory.c` | Manages symbol tables, scope ladders, and arena allocations. |
| **Error Handling** | `src/error.c`, `headers/error.h` | Formats and tracks non-halting syntax and semantic errors. |

---

## Coding and Design Guidelines

1. **C99 Standard Compatibility**: Write clean, portable C code.
2. **LLVM Basic Block Termination**: When working on control flow in `src/llvm_backend.c` (`if`, `while`, `for`), every LLVM basic block **must** end with a valid terminator instruction (`LLVMBuildBr`, `LLVMBuildCondBr`, or `LLVMBuildRet`).
3. **No Silent Error Suppression**: Always use the built-in error reporting stack (`report_semantic_error()`, `error()`) rather than silently failing.
4. **AST Safety**: Ensure union field accesses on `ASTNode` match `node->Type` or `node->var_assign.value->Type`.
5. **Memory Management**: Allocate compiler nodes through `alloc_space()` to ensure lifetime tracking and proper cleanup.

---

## How to Submit a Contribution

1. **Fork the Repository** and create a feature branch (`git checkout -b feature/my-new-feature`).
2. **Commit your changes** with clear, descriptive commit messages.
3. **Verify compilation & test suite**: Make sure `apl.c` builds cleanly without warnings and that all test `.apl` files execute successfully.
4. **Push to your branch** (`git push origin feature/my-new-feature`).
5. **Open a Pull Request** explaining what changes were made and why.

Thank you for helping build Apollo.
