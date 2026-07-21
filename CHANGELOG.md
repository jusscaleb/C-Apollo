# Changelog

All notable changes to the Apollo Programming Language project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
