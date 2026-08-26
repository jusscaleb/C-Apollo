#include "apl-mem.h"


// Helper to allocate an Arena Chunk
static AplArenaChunk *create_chunk(size_t capacity) {
    AplArenaChunk *chunk = (AplArenaChunk *)malloc(sizeof(AplArenaChunk));
    if (!chunk) return NULL;

    chunk->buffer = (char *)malloc(capacity);
    if (!chunk->buffer) {
        free(chunk);
        return NULL;
    }

    chunk->capacity = capacity;
    chunk->used = 0;
    chunk->next = NULL;
    return chunk;
}

// Creates an Arena with Block Chaining (64KB default capacity)
AplArena *apl_arena_create(size_t capacity) {
    if (capacity == 0) capacity = DEF_CAPACITY; // Default to 64KB chunks
    AplArena *arena = (AplArena *)malloc(sizeof(AplArena));
    if (!arena) return NULL;

    AplArenaChunk *initial_chunk = create_chunk(capacity);
    if (!initial_chunk) {
        free(arena);
        return NULL;
    }

    arena->first = initial_chunk;
    arena->current = initial_chunk;
    arena->default_capacity = capacity;
    return arena;
}

// Cold-path function called ONLY when current chunk capacity is exceeded
void *apl_arena_grow_and_alloc(AplArena *arena, size_t aligned_size) {
    if (!arena) return NULL;

    size_t chunk_cap = arena->default_capacity;
    if (aligned_size > chunk_cap) {
        chunk_cap = aligned_size;
    }

    // Reuse existing next chunk in the chain if available
    if (arena->current->next) {
        arena->current = arena->current->next;
        arena->current->used = 0;
        if (arena->current->capacity < aligned_size) {
            char *new_buf = (char *)realloc(arena->current->buffer, aligned_size);
            if (new_buf) {
                arena->current->buffer = new_buf;
                arena->current->capacity = aligned_size;
            }
        }
    } else {
        // Append a new chunk without moving/invalidating existing pointers
        AplArenaChunk *new_chunk = create_chunk(chunk_cap);
        if (!new_chunk) return NULL;

        arena->current->next = new_chunk;
        arena->current = new_chunk;
    }

    void *ptr = arena->current->buffer + arena->current->used;
    arena->current->used += aligned_size;
    return ptr;
}

// Wipes all chunks back to 0 without freeing memory buffers
void apl_arena_reset(AplArena *arena) {
    if (!arena) return;

    AplArenaChunk *chunk = arena->first;
    while (chunk) {
        chunk->used = 0;
        chunk = chunk->next;
    }
    arena->current = arena->first;
}

// Frees all chunk buffers and the arena at process exit
void apl_arena_destroy(AplArena *arena) {
    if (!arena) return;

    AplArenaChunk *chunk = arena->first;
    while (chunk) {
        AplArenaChunk *next = chunk->next;
        if (chunk->buffer) free(chunk->buffer);
        free(chunk);
        chunk = next;
    }
    free(arena);
}

// Allocates in Bucket 3 Heap with ref_count = 1
void *apl_heap_alloc_arc(size_t size) {
    size_t total_size = sizeof(AplArcHeader) + size;
    AplArcHeader *header = (AplArcHeader *)malloc(total_size);
    if (!header) return NULL;

    header->ref_count = 1;
    header->size = size;
    return (void *)(header + 1);
}

// Returns current ARC reference count
int64_t apl_arc_ref_count(const void *ptr) {
    if (!ptr) return 0;
    const AplArcHeader *header = ((const AplArcHeader *)ptr) - 1;
    return header->ref_count;
}
