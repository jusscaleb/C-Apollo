# Apollo Compiler Performance & Optimization Guide

This document outlines optimization techniques, architectural improvements, and best practices to maximize both **Compiler Speed (Build Times)** and **Generated Program Runtime Performance (Executable Speed)**.

---

## 1. Compiler Driver & Toolchain Optimizations (Build Time Speed)

### Current vs. Optimal Driver Architecture
Currently, Apollo outputs LLVM bitcode to disk (`temp/output.bc`) and launches Clang via a shell process (`system("clang ...")`) to produce the final executable.

#### Recommendations
1. **Direct Object Code Generation via LLVM TargetMachine (Eliminate Clang subprocess)**
   - Instead of calling `system("clang ...")`, use the LLVM C API to emit native machine code (`.o` / `.obj`) directly from memory:
     ```c
     LLVMTargetRef target;
     char *error = NULL;
     LLVMGetTargetFromTriple(LLVMGetDefaultTargetTriple(), &target, &error);
     LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
         target, LLVMGetDefaultTargetTriple(), "generic", "",
         LLVMCodeGenLevelAggressive, LLVMRelocDefault, LLVMCodeModelDefault
     );
     LLVMTargetMachineEmitToFile(machine, components->module, "output.o", LLVMObjectFile, &error);
     ```
2. **Direct In-Memory Execution / JIT (ExecutionEngine / ORC JIT)**
   - For fast development and immediate script execution (like an interpreter), use LLVM's **ORC JIT** or `LLVMCreateExecutionEngineForModule` to run code directly in memory without writing files to disk.

3. **Compiler Build Flags**
   - Build the `apollo` compiler binary with `-O3 -flto` (Link Time Optimization) to speed up internal AST traversal and lexing routines.

---

## 2. Frontend Optimizations (Lexer & Parser)

### 1. Custom Memory Arena Allocator (Implemented in `src/memory.c`)
- **Implementation**: The compiler uses a **Bump / Arena Allocator** (`Arena`) initialized in `src/main.c` (`arena_init`, `arena_alloc`, `arena_reset`).
- **Benefit**: Allocates a continuous chunk (1MB+) at startup, bumping an offset pointer for AST node allocations. The arena is reset at the end of execution in $O(1)$ time, eliminating thousands of individual `malloc()`/`free()` overheads.


### 2. Fast String Handling & String Interning
- Use a **String Interning Pool** (hash table of unique identifiers and strings) during lexing.
- Compare variable/function names using pointer equality (`node->name == symbol->name`) instead of string comparison (`strcmp` / `memcmp`).

### 3. Keyword Lookup Optimization
- Instead of linear `strcmp` chains for keywords in the lexer, use a **Trie**, **Perfect Hash Function (gperf)**, or a **Bitwise State Machine**.

---

## 3. Generated Binary Runtime Optimizations (Target Executable Speed)

To make compiled `.apl` programs run at peak native performance, apply these runtime optimization strategies:

### 1. LLVM Optimization Pass Pipeline (Mem2Reg & SSA)
By default, standard code generators allocate variables on the stack (`alloca`). Running LLVM optimization passes converts stack accesses into CPU registers:
- **`Mem2Reg` (`LLVMAddPromoteMemoryToRegisterPass`)**: Promotes stack `alloca` memory locations into LLVM SSA registers, enabling the CPU to keep variables in registers instead of reading/writing to RAM.
- **`GVN` (`LLVMAddGVNPass`)**: Global Value Numbering eliminates redundant code and repeated computations.
- **`Instruction Combining` (`LLVMAddInstructionCombiningPass`)**: Simplifies math operations into faster CPU instructions.
- **`CFG Simplification` (`LLVMAddCFGSimplificationPass`)**: Cleans up unused basic blocks and optimizes conditional branch jumps.
- **`Loop Vectorization` (`LLVMAddLoopVectorizePass`)**: Converts loops to use CPU SIMD vector instructions (AVX2/AVX-512).

### 2. Inlining Runtime Library Functions
- Mark functions in `apl-io.c` and `apl-string.c` with `__attribute__((always_inline))` or inline them during bitcode linking.
- When `println()` or string operations are inlined into the main code block, LLVM can optimize away function call frame setups completely.

### 3. Host CPU Architecture Tuning (`-march=native`)
- Specify the host CPU features during LLVM TargetMachine creation using `LLVMGetHostCPUName()` and `LLVMGetHostCPUFeatures()`. This allows LLVM to generate optimized SIMD and bit-manipulation instructions specific to the user's processor.

### 4. Optimized I/O Buffering (`apl-io`)
- Replace unbuffered character/line printing with a custom ring buffer in `apl-io`. Flush stdout only when the buffer is full or when program execution finishes.

---

## 4. Summary Checklist for Maximum Performance

| Phase | Strategy | Expected Performance Impact |
| :--- | :--- | :--- |
| **Driver (Build Time)** | Disable AddressSanitizer (`-fsanitize=address`) in release mode | **10x – 20x faster compilation** |
| **Driver (Build Time)** | Use Direct `LLVMTargetMachineEmitToFile` instead of Clang shell process | **2x – 3x faster compilation** |
| **Frontend (Memory)** | Implement AST Arena Allocator (`malloc` reduction) | **20% – 40% lower frontend memory overhead** |
| **Frontend (Lexer)** | String Interning & Pointer Comparisons | **15% – 30% faster parsing** |
| **Runtime (CodeGen)** | LLVM `Mem2Reg` + SSA Optimization Pass Pipeline | **2x – 5x faster target binary execution** |
| **Runtime (Target CPU)**| Host CPU Feature Targeting (`LLVMGetHostCPUName`) | **SIMD vectorization & faster loops** |
| **Runtime (I/O)** | Buffered I/O in `apl-io` | **10x faster printing in loops** |
