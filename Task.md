# Apollo OOP Migration Checklist

To evolve Apollo from a procedural language into a fully Object-Oriented Programming (OOP) language, you need to implement Data Encapsulation (Classes/Structs), Methods, Member Access, and Instantiation. 

Here is the step-by-step checklist of everything you need to code:

## Phase 1: Lexical Foundations
- [ ] **New Keywords:** Update `src/lexer.c` and `headers/token.h` to recognize `class`, `new`, `this` (or `self`), `public`, and `private`.
- [ ] **The Dot Operator:** Add support for the `.` token (e.g., `TOKEN_DOT`) so users can access fields and methods (`obj.field`).

## Phase 2: Data Encapsulation (Classes/Structs)
- [ ] **AST Nodes:** Create `AST_CLASS_DECL` in `ast.h` to hold a class name, its properties (variables), and its methods.
- [ ] **Parser:** Update `src/parser.c` to parse `class Name { ... }` blocks.
- [ ] **Semantic Analyzer:** Update `src/semantic.c` to register the Class in the Symbol Table. You will need to calculate and store the memory size of the class based on its fields.
- [ ] **LLVM IR Generation:** In `src/generator.c`, translate classes into LLVM Struct types (e.g., `%ClassType = type { i32, float, i8* }`).

## Phase 3: Object Instantiation (Memory Allocation)
- [ ] **AST Nodes:** Create `AST_INSTANTIATE` for `new ClassName()`.
- [ ] **Parser:** Update the expression parser to handle the `new` keyword.
- [ ] **LLVM IR Generation:** When encountering `new`, emit a `malloc` call in LLVM for the exact byte-size of the class struct, and return the pointer.

## Phase 4: Methods and the `this` Pointer
- [ ] **Parser:** Allow functions (`fxn`) to be declared inside a `class` block.
- [ ] **Semantic Analyzer:** Scoping is critical here. When a method is called, the compiler needs to know it belongs to the class scope, not the global scope.
- [ ] **LLVM IR Generation (The Secret to OOP):** Under the hood, class methods are just regular global functions where the *very first hidden parameter* is a pointer to the object instance (`this`). Update codegen to automatically pass the object pointer into the method!

## Phase 5: Member Access (The Dot Operator)
- [ ] **AST Nodes:** Create `AST_MEMBER_ACCESS` for syntax like `player.health`.
- [ ] **Semantic Analyzer:** Check the symbol table to verify that `player` is an object, and that `health` actually exists on that object.
- [ ] **LLVM IR Generation:** Use the LLVM `getelementptr` (GEP) instruction. This instruction takes the base pointer (`player`) and calculates the exact memory offset to find the specific field (`health`).

## Phase 6 (Advanced): Inheritance & Polymorphism
- [ ] **Inheritance Syntax:** Allow classes to inherit properties (e.g., `class Dog : Animal`).
- [ ] **Struct Embedding:** In LLVM, implement inheritance by making the Parent struct the very first field of the Child struct.
- [ ] **Virtual Methods (V-Tables):** To support overriding methods, create an array of function pointers (a V-Table) for each class so the program knows which version of a method to execute at runtime.
