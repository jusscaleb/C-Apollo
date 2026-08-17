# Apollo Runtime Modules (`apollo-modules`)

The `apollo-modules` directory contains Apollo's standard runtime sub-libraries written in C and pre-compiled into LLVM Bitcode (`.bc`).

During LLVM code generation, Apollo's compiler backend (`src/API/llvm_main.c`) dynamically imports and links these bitcode modules via `LLVMLinkModules2`. This provides low-level I/O, string operations, and system interop directly inside compiled Apollo binaries without needing external dynamic link libraries (DLLs) at runtime.

---

## Directory Structure

```text
apollo-modules/
├── APLMODULES.md       # Runtime module documentation and specification
├── compile.sh          # Shell script to compile C runtime modules to LLVM bitcode (.bc)
├── apl-io/             # Standard I/O module
│   ├── apl-io.c        # I/O implementation (console output, formatting)
│   ├── apl-io.h        # I/O function prototypes and declarations
│   └── apl-io.bc       # Compiled LLVM Bitcode linked during code generation
├── apl-string/         # String manipulation module
│   ├── apl-string.c    # String allocation, concatenation, length & slice functions
│   ├── apl-string.h    # String module function declarations
│   └── apl-string.bc   # Compiled LLVM Bitcode linked during code generation
└── apl-sys/            # System & OS interop module
    ├── apl-sys.c       # System level calls and memory management wrappers
    ├── apl-sys.h       # System module declarations
    └── apl-sys.bc      # Compiled LLVM Bitcode linked during code generation
```

---

## Module Specifications

### 1. `apl-io` (Input / Output Primitives)
- **Files**: `apl-io/apl-io.c`, `apl-io/apl-io.h`, `apl-io/apl-io.bc`
- **Purpose**: Provides formatted console printing for scalar types (`int`, `float`, `bool`, `str`).
- **Core Functions**:
  - `print_int(int value)`: Outputs integer values to `stdout`.
  - `print_float(double value)`: Outputs floating-point values to `stdout`.
  - `print_bool(int value)`: Outputs `true` or `false` booleans.
  - `print_str(const char *str)`: Outputs null-terminated string slices.
  - `print_newline()`: Emits a newline (`\n`).

### 2. `apl-string` (String Operations)
- **Files**: `apl-string/apl-string.c`, `apl-string/apl-string.h`, `apl-string/apl-string.bc`
- **Purpose**: Low-level string memory management and manipulation functions.
- **Core Functions**:
  - String concatenation and slice allocation.
  - String comparison and length calculations.

### 3. `apl-sys` (System & OS Interop)
- **Files**: `apl-sys/apl-sys.c`, `apl-sys/apl-sys.h`, `apl-sys/apl-sys.bc`
- **Purpose**: System-level utilities, heap memory allocation wrappers, and OS process helpers.

---

## Re-Compiling Runtime Bitcode Modules

If you modify any `.c` source files inside `apl-io/`, `apl-string/`, or `apl-sys/`, you must re-compile them to LLVM bitcode (`.bc`) so the Apollo compiler backend can link the updated implementations.

Use the `compile.sh` helper script:

```bash
# Navigate to the apollo-modules directory
cd apollo-modules

# Compile individual modules to bitcode (.bc)
./compile.sh apl-io
./compile.sh apl-string
./compile.sh apl-sys
```

### Manual Bitcode Compilation Command

Under the hood, `compile.sh` invokes Clang to compile C files to LLVM IR bitcode:

```bash
clang -emit-llvm -O3 -flto -ffunction-sections -fdata-sections -DNDEBUG -c apl-io/apl-io.c -o apl-io/apl-io.bc
```

---

## Compiler Integration

In `src/API/llvm_main.c`, runtime modules are imported automatically when the compiler initializes:

```c
void _apl_load_runtime_libraries(LLVMComponents *components) {
    _apl_import_runtime(components->module, "apollo-modules/apl-io/apl-io.bc");
    _apl_import_runtime(components->module, "apollo-modules/apl-string/apl-string.bc");
    _apl_import_runtime(components->module, "apollo-modules/apl-sys/apl-sys.bc");
}
```
