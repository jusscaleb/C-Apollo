# Patch Plan: v3.1.2

This document tracks fixes identified during the codebase review for the next patch release. The items below are bug fixes; no new language features are implied.

## Compiler

- [ ] **Prevent collisions between method and function names** (`src/Semantics/semantic_analyze.c`, method registration and lookup)
  - Method calls are resolved by combining the struct and method names with an underscore (for example, `Point.move` becomes `Point_move`). A user-defined function can have that same valid identifier, so the two symbols collide and a method call can resolve to the wrong function.
  - Use a compiler-only representation for method symbols and make both registration and lookup use it. Do not rely on a separator that Apollo identifiers can also contain.
  - **Regression case:** define `Point.move(self) -> int` to return `1` and a free `Point_move() -> int` to return `99`; call both from `run()`.
  - **Acceptance:** both declarations can coexist, `p.move()` returns `1`, and `Point_move()` returns `99`.

- [ ] **Tokenize minus consistently** (`src/lexer.c`, `src/Parser/parser_expr.c`)
  - Always emit `-` as a subtraction token, including when it is immediately followed by a digit.
  - Parse negative values as unary minus so `x-5`, `x - 5`, and `-5` all work consistently.
  - **Acceptance:** whitespace does not change the meaning or validity of subtraction; unary negative integer and float literals still compile correctly.

- [ ] **Fix function-name buffer bounds in semantic analysis** (`src/Semantics/semantic_analyze.c`)
  - Allocate room for the copied function name and its terminating NUL byte.
  - Check a name's length before comparing it with the seven-character built-in name `println`.
  - **Acceptance:** calls with short names and calls to `println` are analyzed without reading or writing beyond the temporary name buffer.

- [ ] **Grow AST block statement lists using the arena** (`src/ast.c`)
  - `create_block_node` allocates the statement list from the compiler arena. Replace the `realloc` growth path with a larger arena allocation and a copy of the existing entries.
  - Handle allocation failure consistently with other AST allocations.
  - **Acceptance:** parsing a block with more than eight statements does not pass arena memory to `realloc` and preserves every statement.

- [ ] **Correct lexer source locations** (`src/lexer.c`, `headers/token.h`)
  - Record each token's starting column and advance the column once per consumed character.
  - Keep line and column tracking consistent across newlines, comments, strings, and multi-character tokens.
  - **Acceptance:** tokens and diagnostics after punctuation and comments report the actual source line and column.

- [ ] **Validate source file size handling** (`src/main.c`)
  - Check `fseek`, `ftell`, and `fread` results before allocating or using the source buffer; reject failed or unrepresentable file sizes cleanly.
  - **Acceptance:** an unreadable, empty, or failed/oversized input read exits with a clear error rather than using an invalid size or partial buffer silently.

## Runtime modules

- [ ] **Give concatenated strings storage that outlives the helper call** (`apollo-modules/apl-string/apl-string.c`)
  - Integer, boolean, and float concatenation currently point the result `String` at a local stack array. Store the result in memory with a documented lifetime that remains valid after the function returns.
  - **Acceptance:** concatenated results remain readable after return and after subsequent helper calls, for the documented lifetime.

- [ ] **Fix string search loop condition** (`apollo-modules/apl-string/apl-string.c`)
  - Iterate from index zero up to, but not including, the string length.
  - **Acceptance:** searching finds characters at the first, middle, and last positions, and returns `-1` when absent or when the string is empty.

- [ ] **Fix `char_at` bounds checking** (`apollo-modules/apl-string/apl-string.c`)
  - Reject negative indexes and indexes greater than or equal to the string length.
  - **Acceptance:** valid indexes return their characters; `-1` and `length` are treated as out of range without reading outside the string contents.

- [ ] **Correct float concatenation length calculation** (`apollo-modules/apl-string/apl-string.c`)
  - Calculate formatted length from the actual start and end of the generated text, rather than the unrelated `float_buf + 11` position.
  - **Acceptance:** positive and negative float values produce a correctly sized string without truncation or invalid copy lengths.

- [ ] **Handle signed integer edge cases in numeric formatting** (`apollo-modules/apl-string/apl-string.c`)
  - Avoid negating the minimum signed integer in its signed type when computing an absolute value.
  - **Acceptance:** formatting the minimum and maximum supported integers does not invoke signed overflow and produces the expected text.

- [ ] **Guard runtime arena size arithmetic** (`apollo-modules/apl-mem/apl-mem.h`, `apollo-modules/apl-mem/apl-mem.c`)
  - Check for overflow before alignment and before adding the requested size to the current usage. Ensure growth allocation failure is reported safely and never returns a pointer without enough capacity.
  - **Acceptance:** oversized requests fail cleanly; normal allocations, growth, and reset continue to work without wrapping size calculations.

## Verification

- [ ] Add or update focused regression cases for each acceptance criterion above.
- [ ] Run the compiler and runtime test suites on the supported Windows/MinGW/LLVM toolchain.
- [ ] Confirm the changelog and version metadata identify the release as v3.1.2.
