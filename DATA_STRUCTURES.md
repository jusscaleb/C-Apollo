# Apollo Data Structures Roadmap & Architecture Specification

**Version:** `v3.0.0`  
**Status:** Active Specification  
**Memory Integration:** 3+1 Bucket Memory Architecture  

This document outlines the core data structures required for Apollo to evolve into a modern, production-grade compiled programming language. Each data structure is mapped directly to Apollo's **3+1 Bucket Memory Model** to guarantee near-zero runtime allocation latency and zero Garbage Collection (GC) pauses.

---

## Executive Summary: 3+1 Memory Bucket Mapping

| Data Structure | Primary Memory Bucket | LLVM IR Representation | Allocation Latency | Deallocation Latency |
| :--- | :--- | :--- | :--- | :--- |
| **Static Array (`T[N]`)** | **Bucket 0 (+1)** *(Call Stack)* | `[N x ElementType]` | **0.0 ns** *(Hardware `alloca`)* | **0.0 ns** *(Stack Pop)* |
| **Dynamic Array (`T[]`)** | **Bucket 2** *(Scoped Arena)* | `%ArrayStruct = type { ptr, i32, i32 }` | **~1 – 2 ns** *(Arena Bump)* | **0.0 ns** *(Bulk Arena Reset)* |
| **Hash Dictionary (`dict`)** | **Bucket 2** *(Scoped Arena)* | `%DictStruct = type { ptr*, i32, i32 }` | **~1 – 2 ns** *(Arena Bump)* | **0.0 ns** *(Bulk Arena Reset)* |
| **Class / Struct (`class`)**| **Bucket 0 / 2 / 3** | `%ClassStruct = type { ... }` | Variable by bucket | Variable by bucket |
| **String Slice (`str_view`)**| **Bucket 0 / 1** *(Zero-Copy)* | `%SliceStruct = type { i8*, i32 }` | **0.0 ns** *(No Heap Copy)* | **0.0 ns** |
| **Tuple Pair (`(T1, T2)`)** | **Bucket 0 (+1)** *(Call Stack)* | `{ Type1, Type2 }` | **0.0 ns** *(Registers / Stack)* | **0.0 ns** *(Stack Pop)* |

---

## 1. Core Data Structures & Syntax

### 1.1 Static Fixed-Size Arrays (`T[N]`)

Static arrays have a fixed length known at compile time `N`. They reside entirely on the **CPU Call Stack (Bucket 0)**.

#### **Syntax Specification**:
```apl
// Declaring a fixed 5-element static array
int[5] scores = [90, 85, 95, 88, 100];

// Element access and mutation
scores[0] = 92;
println(scores[0]);
```

#### **C Structural Representation**:
```c
typedef struct {
    DataType element_type;
    uint32_t fixed_length;
    bool is_dynamic; // false
} StaticArrayMeta;
```

#### **LLVM IR Code Generation**:
```llvm
; Allocated on CPU Call Stack in Bucket 0
%scores = alloca [5 x i32], align 4

; Element Access (scores[0] = 92)
%elem_ptr = getelementptr inbounds [5 x i32], ptr %scores, i64 0, i64 0
store i32 92, ptr %elem_ptr, align 4
```

---

### 1.2 Dynamic Resizable Arrays (`T[]` / `Array<T>`)

Dynamic arrays can grow and shrink at runtime (`push`, `pop`, `append`). They reside in the **Scoped Region Arena (Bucket 2)** for zero-GC $O(1)$ allocation and instant batch cleanup.

#### **Syntax Specification**:
```apl
// Declaring a dynamic array in Bucket 2 Arena
int[] dyn_list = [10, 20, 30];

// Resizing operations
dyn_list.push(40);
println("Length: ", dyn_list.len());
```

#### **C Structural Representation**:
```c
typedef struct {
    void *elements;     // Pointer to contiguous buffer in Bucket 2 Arena
    uint32_t length;    // Active element count
    uint32_t capacity;  // Allocated buffer capacity
} AplArrayHeader;
```

#### **LLVM IR Code Generation**:
```llvm
; Dynamic Array Slice Header allocated in Bucket 2 Arena
; Layout: { ptr elements, i32 length, i32 capacity }
%AplArray = type { ptr, i32, i32 }

%dyn_list = call ptr @apl_arena_grow_and_alloc(ptr %arena, i64 16)
```

---

### 1.3 Hash Dictionaries / Maps (`dict` / `Map<K, V>`)

Key-value hash maps providing $O(1)$ average lookups. Bucket arrays and collision chain entries are stored in **Bucket 2 Arena**.

#### **Syntax Specification**:
```apl
// Declaring a dictionary map
dict user_scores = { "Caleb": 100, "Alex": 95 };

// Map operations
user_scores["Jordan"] = 88;
println("Caleb Score: ", user_scores["Caleb"]);
```

#### **C Structural Representation**:
```c
typedef struct AplDictNode {
    void *key;
    void *value;
    uint32_t hash;
    struct AplDictNode *next;
} AplDictNode;

typedef struct {
    AplDictNode **buckets; // Array of collision chains in Bucket 2 Arena
    uint32_t capacity;     // Bucket size (e.g. 16, 32, 64)
    uint32_t count;        // Total key-value pairs
} AplDictHeader;
```

#### **Why Apollo Outperforms Java/Python Maps**:
In Java and Python, deleting or freeing a Hash Map requires traversing linked lists and invoking `free()` / GC on every individual entry node. In Apollo, all map entry nodes are allocated inside the **Bucket 2 Arena**. Exiting the function frame wipes the entire map instantly in **0 nanoseconds**.

---

### 1.4 User-Defined Structs & Classes (`class` / `struct`)

Supports Data Encapsulation, Member Access (`.`), Methods, and Object Instantiation (`new`).

#### **Syntax Specification**:
```apl
class Player {
    str name;
    int health;

    fxn take_damage(int amount) {
        this.health -= amount;
    }
}

fxn run() {
    // Stack Struct (Bucket 0)
    Player p1 = Player{ name: "Caleb", health: 100 };

    // Escaping Heap ARC Object (Bucket 3)
    Player* p2 = new Player{ name: "Boss", health: 500 };
}
```

#### **LLVM IR Code Generation**:
```llvm
; Defined as an LLVM Anonymous Struct Type
%Player = type { %String, i32 }
```

---

### 1.5 Zero-Copy String Slices (`str_view`)

Allows extracting substrings or tokenizing strings without allocating new heap memory or copying byte buffers.

#### **Syntax Specification**:
```apl
str full_text = "Apollo Engine";
str_view sub = full_text.slice(0, 6); // Points to "Apollo" with 0 allocation
```

#### **C Structural Representation**:
```c
typedef struct {
    const char *ptr;   // Direct pointer into original string buffer
    uint32_t length;   // Length of slice
} AplStrView;
```

---

## 2. Compiler AST Node Definitions (`headers/ast.h`)

To support these data structures in the frontend, add the following node types and union members to [`headers/ast.h`](file:///c:/Users/caleb/SCXRPIUS/Apollo/headers/ast.h):

### **AST Node Type Enumeration**:
```c
typedef enum {
    // ... existing AST types ...
    AST_ARRAY_LITERAL, // Handles [1, 2, 3]
    AST_MAP_LITERAL,   // Handles { "key": "value" }
    AST_INDEX_EXPR,    // Handles arr[i] or dict["key"]
    AST_MEMBER_ACCESS, // Handles obj.field
    AST_STRUCT_DECL,   // Handles struct / class declarations
    AST_INSTANTIATE,   // Handles new ClassName()
} ASTNodeType;
```

### **AST Node Union Extensions**:
```c
struct ASTNode {
    ASTNodeType Type;

    union {
        // ... existing union members ...

        // Array Literal Node ([10, 20, 30])
        struct {
            ASTNode **elements;    // Array of element ASTNode pointers
            int count;             // Number of elements
            int capacity;          // Allocated capacity
            DataType element_type; // Type of elements
            bool is_dynamic;       // false = Bucket 0 (Static), true = Bucket 2 (Dynamic)
        } array_literal;

        // Map Literal Node ({ "a": 1 })
        struct {
            ASTNode **keys;        // Array of key ASTNodes
            ASTNode **values;      // Array of value ASTNodes
            int count;             // Key-value count
            DataType key_type;
            DataType value_type;
        } map_literal;

        // Index Access Node (arr[i] or dict[key])
        struct {
            ASTNode *target;       // Variable/target node
            ASTNode *index;        // Index/Key expression node
            DataType result_type;  // Type of retrieved element
        } index_expr;

        // Member Access Node (player.health)
        struct {
            ASTNode *target;       // Object instance node
            const char *member_name;
            int member_length;
            int field_index;       // Struct GEP element offset index
        } member_access;
    };
};
```

---

## 3. Implementation Checklist Roadmap

- [ ] **Phase 1: Static Arrays (`T[N]`)**
  - Add `AST_ARRAY_LITERAL` and `AST_INDEX_EXPR` to `headers/ast.h`.
  - Update `src/Parser/parser_expr.c` to parse `[1, 2, 3]` and `arr[i]`.
  - Implement LLVM stack allocation (`alloca [N x T]`) and GEP indexing in `src/API/llvm_variables.c`.

- [ ] **Phase 2: Dynamic Arrays (`T[]`)**
  - Add Bucket 2 Arena slice header allocation in `src/API/llvm_memory.c`.
  - Implement runtime array methods (`.push()`, `.len()`, `.pop()`) in `apollo-modules/apl-array`.

- [ ] **Phase 3: Class & Struct Encapsulation (`struct` / `class`)**
  - Add `AST_STRUCT_DECL` and `AST_MEMBER_ACCESS` to `headers/ast.h`.
  - Translate classes into LLVM Struct types (`LLVMStructTypeInContext`).
  - Implement Member Access dot operator (`.`) via LLVM `LLVMBuildGEP2`.

- [ ] **Phase 4: Hash Dictionaries (`dict`)**
  - Implement Arena-backed collision chain hash map allocation in `apollo-modules/apl-dict`.
  - Connect parser map literals `{ "k": "v" }` to dictionary codegen.
