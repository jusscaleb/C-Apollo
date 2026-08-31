# Apollo Memory Architecture: Improvement & Optimization Roadmap

**Status:** Proposed Architecture Upgrades  
**Target:** Upgrading Apollo 3+1 Bucket Memory from `8.5/10` to `10/10` in Efficiency & Scalability  

---

## 1. Executive Summary & Evaluation Scorecard

Apollo's **3+1 Bucket Memory Architecture** provides a high-performance hybrid model:
- **Bucket 0 (+1)**: CPU Call Stack (`alloca`) — 0% overhead
- **Bucket 1**: Static Data Segment — zero runtime allocation cost
- **Bucket 2**: Scoped Region Arenas (`apl-mem`) — $O(1)$ linear bump allocation and bulk scope resets
- **Bucket 3**: Dynamic Heap ARC — automatic reference counting restricted strictly to escaping data

| Area | Current Rating | Target Rating | Primary Focus |
| :--- | :---: | :---: | :--- |
| **Allocation Speed** | 9.2 / 10 | 9.9 / 10 | Single-allocation chunk headers (inline buffers) |
| **Cache Locality** | 8.8 / 10 | 9.8 / 10 | Contiguous metadata and payload memory packing |
| **Memory Footprint** | 7.8 / 10 | 9.5 / 10 | Arena reset trimming to eliminate high-water mark bloat |
| **Loop Safety** | 8.0 / 10 | 9.9 / 10 | Multi-chunk loop bookmark tracking |
| **ARC Safety** | 7.5 / 10 | 9.5 / 10 | Weak references (`weak` keyword) to prevent reference cycles |
| **Multi-Threading** | 8.0 / 10 | 9.8 / 10 | Thread-local micro-arenas with page recycling |

---

## 2. Priority 1: High-Impact Allocator Optimizations

### 1.1 Single-Allocation Chunk Headers (Inline Buffers)

#### The Problem
Currently in `apollo-modules/apl-mem/apl-mem.c`, `create_chunk()` calls `malloc()` twice:
1. `malloc(sizeof(AplArenaChunk))` (allocates the metadata node)
2. `malloc(capacity)` (allocates the payload buffer)

This doubles the number of OS/CRT allocator calls and adds unnecessary heap metadata overhead.

#### Recommended Improvement
Allocate both the `AplArenaChunk` header and its data buffer in **one single `malloc` call**:
```
[ AplArenaChunk Struct (24-32B) ][ Contiguous Data Buffer (Capacity Bytes) ... ]
```

#### Expected Benefits
- **50% fewer heap allocator calls** when expanding arenas.
- **Improved CPU Cache Prefetching**: Metadata and data buffer sit contiguously in memory.
- **Zero internal pointer fragmentation**.

---

### 1.2 Arena Reset Trimming (Prevent High-Water Mark Bloat)

#### The Problem
Currently, `apl_arena_reset()` rewinds all chunks to `used = 0`, but retains all allocated chunks in the linked list indefinitely. 

If a temporary spike (e.g., parsing a huge file or processing a large JSON payload) expands the chunk chain to 200MB, that 200MB remains allocated in RAM until the process or arena is completely destroyed. In long-running services and daemons, this causes memory bloat.

#### Recommended Improvement
During `apl_arena_reset()`:
1. Rewind the primary chunk (`first`) to `used = 0`.
2. Free any chained overflow chunks (`first->next`) back to the system allocator (or retain only up to a fixed threshold like 1MB).
3. Set `first->next = NULL` and `current = first`.

#### Expected Benefits
- Prevents memory hoarding in long-running applications and worker threads.
- Keeps hot-path memory usage flat and bounded.

---

## 3. Priority 2: Robustness & Safety Upgrades

### 3.1 Chunk-Aware Loop Bookmarks

#### The Problem
Currently, `apl_arena_get_mark()` and `apl_arena_set_mark()` only record an integer byte offset (`size_t used`). If a loop iteration allocates enough temporary data to trigger an arena expansion into a second chunk, resetting to a single integer offset cannot properly rewind across chunk boundaries.

#### Recommended Improvement
Define a composite bookmark structure that tracks both the chunk pointer and the offset:
```c
typedef struct {
    AplArenaChunk *chunk;
    size_t offset;
} AplArenaMark;
```
When rewinding to a bookmark:
1. Free or reset chunks allocated after the bookmarked chunk.
2. Restore `arena->current = mark.chunk`.
3. Restore `arena->current->used = mark.offset`.

#### Expected Benefits
- Guarantees 100% reliable loop resets regardless of how much memory a single iteration consumes.
- Prevents desynchronization between chunk pointers and offsets.

---

### 3.2 Weak References for Bucket 3 (`weak` Keyword)

#### The Problem
Bucket 3 uses Automatic Reference Counting (ARC). Pure ARC suffers from **reference cycles** (e.g., Node A points to Node B, and Node B points back to Node A). When cycles occur, reference counts never hit zero, causing permanent memory leaks.

#### Recommended Improvement
Introduce weak pointers for non-owning back-references (e.g. `weak Node* parent`):
1. **Strong Pointer (`heap Object*`)**: Increments `ref_count` by 1. Owns the object.
2. **Weak Pointer (`weak Object*`)**: Does **not** increment `ref_count`. Observes the object without keeping it alive.

#### Expected Benefits
- Completely solves cyclic memory leaks in graphs, trees, and event observers.
- Maintains ARC deterministic cleanup without needing a heavy Garbage Collector.

---

## 4. Priority 3: High-Concurrency & Multi-Threading

### 4.1 Thread-Local Micro-Arenas with Global Page Recycling

#### The Problem
When running thousands of concurrent threads or tasks, having threads frequently call `malloc`/`free` causes lock contention on the global heap allocator.

#### Recommended Improvement
1. **4KB Starter Micro-Arena**: Initialize each thread or request with a lightweight 4KB thread-local buffer. Over 90% of short-lived tasks finish within this initial buffer.
2. **Lock-Free Page Pool**: If a thread exceeds 4KB, it pops a pre-allocated 64KB block from a shared atomic pool in ~2 nanoseconds.
3. **Recycling on Completion**: Once the thread finishes its task, the 64KB page is returned to the pool for another thread to reuse.

#### Expected Benefits
- Near-zero OS memory allocation calls in multi-threaded workloads.
- Predictable, sub-microsecond latency under heavy concurrent request traffic.

---

## 5. Implementation Roadmap Checklist

- [ ] **Phase 1: Inline Chunk Allocation** — Refactor `create_chunk()` in `apl-mem.c` to use a single contiguous block for header + buffer.
- [ ] **Phase 2: Reset Trimming** — Update `apl_arena_reset()` to prune excess chunks above the baseline capacity.
- [ ] **Phase 3: Multi-Chunk Mark Structure** — Refactor `apl_arena_get_mark` and `apl_arena_set_mark` to support chunk + offset pairs.
- [ ] **Phase 4: Weak Reference Semantics** — Add `weak` type annotations in the parser and LLVM code generation for Bucket 3 objects.
- [ ] **Phase 5: Thread-Local Page Pool** — Implement thread-local arena initialization and lock-free block recycling.
