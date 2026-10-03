# Changelog

All notable changes to the Apollo Programming Language project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [3.0.1] - 2026-10-03 (Patch Update)

### Fixed
*Addressed test suite failures and edge-case discrepancies discovered during diagnostic verification. As this is an active preview release, additional dormant issues may persist.*

- **Struct Member Access**: Handled `AST_ACCESS` in LLVM code generation to resolve "Unsupported statement type" errors during struct field evaluation.
- **Array Element Reassignment**: Resolved false syntax errors previously triggered when mutating existing array elements.


## [3.0.0] - 2026-09-23 (Unstable Preview)

### Added
- **LLVM C API Backend Pipeline**: Completely transitioned compiler code generation from manual string-based LLVM IR formatting to the native LLVM C API (`headers/llvm_backend.h` and modular `src/API/llvm_*.c`), providing robust AST-to-IR generation, type verification, and native optimization.
- **3+1 Bucket Memory Architecture**: Implemented Apollo's deterministic memory management model across four tiers:
  - **Bucket 0 (+1) - CPU Call Stack (`alloca`)**: Zero-overhead allocation for fixed scalar primitives and static fixed-size arrays.
  - **Bucket 1 - Static Bucket**: Compile-time constant allocation in the global data segment for immutable strings and globals.
  - **Bucket 2 - Scoped Region Arena**: Bump-allocated scoped regions with instantaneous $O(1)$ bulk resets on scope exit and loop bookmarking.
  - **Bucket 3 - Dynamic Heap ARC**: Prepend-header automatic reference counting for escaping or dynamically allocated objects.
- **Apollo Runtime Modules (`apollo-modules/`)**: Modular precompiled LLVM bitcode runtime libraries linked during native binary creation:
  - `apl-mem`: Runtime bucket allocation and memory management routines.
  - `apl-io`: High-performance formatted console I/O, `println`, and stdin input (`__apl_input__`).
  - `apl-string`: Dynamic string concatenation (`__apl_str_concat__`), conversions, and formatting.
  - `apl-sys`: Platform system calls and process execution utilities.
- **Arrays & Indexing**: Native support for static fixed-size arrays (`int[N]`) and dynamic arrays (`T[]`), including element indexing (`arr[i]`), mutation (`arr[i] = val`), and bounds-checking infrastructure.
- **Structs**: First-class user-defined composite data structures (`struct Point { int x; int y; }`) with member initialization, stack/heap layout, and dot-access operator (`p.x`).
- **Pointers & References**: First-class pointer declaration, address-of (`&`), and dereference (`*`) operators.
- **Character Datatype (`char`)**: Added dedicated `char` primitive type support with single-quote character literal parsing (`'a'`).
- **Variable Reassignment**: Full support for mutable variables and post-declaration assignment statements (`x = 20;`, string reassignment).
- **Parenthesized Arithmetic Expressions**: Support for grouped arithmetic operations with correct operator precedence (e.g. `(a + b) * c`).
- **Looping Constructs**: Native `while` and `for` loop control flow constructs with loop-level scoped arena resets.
- **Gperf Perfect Keyword Hashing**: Implemented compile-time keyword hash lookup via `src/gperf/keywords.gperf` and `headers/keywords_hash.h` for $O(1)$ keyword identification.
- **Hash Map Symbol Table**: Replaced linear symbol lookup tables with an efficient bucketed hash map implementation for constant-time variable and function resolution.
- **Fast Character Classification Macros**: Replaced standard C library functions (`isdigit`, `isalpha`, `isalnum`) with inlined bitwise macros in `headers/token.h` for faster lexing.
- **Compiler Arena Allocator**: Built an internal bump allocator (`src/memory.c`) for internal compiler data structures and AST nodes to eliminate per-node `malloc`/`free` overhead.
- **Target Machine & Host CPU Tuning**: Added CPU feature detection (`LLVMGetHostCPUName`, `LLVMGetHostCPUFeatures`) and SSA optimization pass pipelines.


### Changed
- **Modularized Parser**: Refactored monolithic `src/parser.c` into dedicated submodules under `src/Parser/` (`parser_entry.c`, `parser_expr.c`, `parser_functions.c`, `parser_data_structures.c`, `parser_var.c`, `parser_helpers.c`).
- **Modularized Semantic Analysis**: Refactored `src/semantic.c` into specialized semantic analyzers under `src/Semantics/` (`semantic_analyze.c`, `semantic_helpers.c`).
- **Modularized Backend Code Generation**: Replaced monolithic text emitter `src/generator.c` with modular LLVM C API modules under `src/API/` (`llvm_main.c`, `llvm_variables.c`, `llvm_fxns.c`, `llvm_condbr.c`, `llvm_arrays.c`, `llvm_structs.c`, `llvm_memory.c`, `llvm_helpers.c`).
- **Nested Function Upvalue Handling**: Improved function builder state stacking and safe upvalue scoping for nested functions.
- **Driver Build Process**: Upgraded driver scripts (`run.sh`, `compiler.sh`) and build configurations (`CMakeLists.txt`) for LLVM 19 and MSYS2 MinGW toolchains.

### Removed
- **Monolithic Text Generator (`src/generator.c`)**: Removed legacy 1,700+ line textual LLVM IR string generation in favor of the LLVM C API.
- **Monolithic Parser & Semantic Files**: Removed legacy single-file `src/parser.c` and `src/semantic.c`.
- **Legacy Driver Helper (`apl.c`)**: Removed obsolete driver helper in favor of direct CLI compiler build flow.

---

## [2.0.0] - 2026-07-21 (Unstable Preview)

### Added
- **Function Parameters & Return Values**: Full support for function parameter passing and explicit return types (`int`, `float`, `str`, `bool`, `void`) using syntax `fxn name(int a, float b) -> (int) { return a; }`.
- **Function Calls in Expressions**: Direct support for evaluating function calls as expressions and argument values (e.g. `println(calc(10))`).
- **Infinite Function Nesting**: Functions can be defined inside other functions with full parent-scope variable visibility and ladder-climbing symbol resolution.
- **Name Mangling**: Automatic scope-based LLVM function name mangling to prevent collisions when re-using function names in nested block scopes.
- **Accurate Line & Column Error Reporting**: Precise `[Line:Column]` location reporting for all semantic compiler errors (`Datatype Mismatch`, `Undeclared Variable`, `Cannot Redefine Function`, etc.).
- **Global Variables**: Support for top-level global variable declarations accessible across all function scopes.
- **Enhanced Primitive Data Types**: First-class support for `int`, `float`, `string`, `bool`, and `null` values.
- **Nested Block Scoping & Shadowing**: Name resolution linking and scope active-state toggling to strictly enforce block scoping rules.
- **String Concatenation**: Binary string concatenation operator (`+`) with support for implicit string conversion of non-string operands (integers, floats, booleans).
- **Control Flow & Loops**: Support for `if`, `elif`, `else` conditional branches alongside `while` and `for` loop constructs.
- **LLVM -O3 Optimization**: Clang integration pipeline compiling generated LLVM IR (`output.ll`) directly into native Windows binaries with `-O3` optimization.

### Fixed
- **Memory Safety & String Lifetime in Parser**: Resolved a Use-After-Free (UAF) bug in `src/parser.c` where function call names were freed prematurely while AST nodes still referenced them.
- **Garbage Column Numbers in Error Stack**: Replaced uninitialized `malloc` memory allocations in error construction with zeroed `calloc` allocations to eliminate random column numbers in compiler output.
- **AST Node Position Tracking**: Ensured line and column numbers are correctly populated across all AST node constructors (`AST_VAR_DECL`, `AST_CALL_FXN`, `AST_VAR_REF`, `AST_BINARY_EXPR`, `AST_BLOCK`, etc.).
- **Panic-Mode Error Recovery**: Fixed cascaded phantom parser errors during syntax error recovery by synchronizing parser state to statement boundaries.

### Changed
- **Multi-Pass Compiler Pipeline**: Upgraded architecture to execute Lexing -> Parsing (AST Construction) -> Semantic Analysis Pass -> LLVM IR Codegen.
- **Compiler Driver Helper (`apl.c`)**: Updated driver helper to orchestrate multi-pass compilation and native executable building seamlessly.

---

## [1.0.0] - Legacy Release

### Added
- Initial procedural compiler implementation.
- Basic lexer, parser, and code generation for primitive arithmetic operations and simple `println` statements.
