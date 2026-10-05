/*===================================================================
                    apl-mem.h (v2.0.0)

                    (c)2026 SCXRPIUS.dev

              The Apollo memory runtime library.
     Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)

----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/




#ifndef APL_MEM_H
#define APL_MEM_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEF_CAPACITY 65536
// =========================================================================
// DATA STRUCTURES
// =========================================================================

// Chunk node for Arena Block Chaining (prevents pointer invalidation)
typedef struct AplArenaChunk {
    char *buffer;
    size_t capacity;
    size_t used;
    struct AplArenaChunk *next;
} AplArenaChunk;

// Bucket 2: Scoped Region Arena Structure
typedef struct AplArena {
    AplArenaChunk *current;
    AplArenaChunk *first;
    size_t default_capacity;
} AplArena;

// Bucket 3: Heap ARC Metadata Header (prepended to heap objects)
typedef struct AplArcHeader {
    int64_t ref_count;
    size_t size;
} AplArcHeader;


// =========================================================================
// FUNCTION PROTOTYPES (COLD PATH)
// =========================================================================

AplArena *apl_arena_create(size_t capacity);
void *apl_arena_grow_and_alloc(AplArena *arena, size_t aligned_size);
void apl_arena_reset(AplArena *arena);
void apl_arena_destroy(AplArena *arena);

void *apl_heap_alloc_arc(size_t size);
int64_t apl_arc_ref_count(const void *ptr);


// =========================================================================
// HOT PATH ALWAYS-INLINED FUNCTIONS
// =========================================================================

#if defined(_MSC_VER)
#define APL_INLINE __forceinline
#define APL_UNLIKELY(x) (x)
#elif defined(__GNUC__) || defined(__clang__)
#define APL_INLINE __attribute__((always_inline))
#define APL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define APL_INLINE inline
#define APL_UNLIKELY(x) (x)
#endif

// Bump-allocates 'size' bytes from the active Arena chunk (2 assembly instructions)
APL_INLINE void *apl_arena_alloc(AplArena *arena, size_t size) {
    size_t aligned_size = (size + 7) & ~((size_t)7);
    if (APL_UNLIKELY(!arena || !arena->current)) return NULL;

    AplArenaChunk *chunk = arena->current;
    if (APL_UNLIKELY(chunk->used + aligned_size > chunk->capacity)) {
        return apl_arena_grow_and_alloc(arena, aligned_size);
    }

    void *ptr = chunk->buffer + chunk->used;
    chunk->used += aligned_size;
    return ptr;
}

// Returns the current offset bookmark inside the active chunk
APL_INLINE size_t apl_arena_get_mark(const AplArena *arena) {
    return (arena && arena->current) ? arena->current->used : 0;
}

// Resets the active chunk to a previous bookmark
APL_INLINE void apl_arena_set_mark(AplArena *arena, size_t mark) {
    if (arena && arena->current && mark <= arena->current->capacity) {
        arena->current->used = mark;
    }
}

// Increments ARC ref_count (ref_count++)
APL_INLINE void apl_arc_retain(void *ptr) {
    if (APL_UNLIKELY(!ptr)) return;
    AplArcHeader *header = ((AplArcHeader *)ptr) - 1;
    __atomic_fetch_add(&header->ref_count, 1, __ATOMIC_RELAXED);
}

// Decrements ARC ref_count (ref_count--). Frees memory instantly when ref_count == 0
APL_INLINE void apl_arc_release(void *ptr) {
    if (APL_UNLIKELY(!ptr)) return;
    AplArcHeader *header = ((AplArcHeader *)ptr) - 1;
    if (__atomic_fetch_sub(&header->ref_count, 1, __ATOMIC_RELEASE) == 1) {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        free(header);
    }
}

#ifdef __cplusplus
}
#endif

#endif // APL_MEM_H
