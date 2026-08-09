#ifndef MEMORY_H
#define MEMORY_H

#define ARENA_ALIGNMENT 8

typedef struct Arena Arena;

typedef struct Arena{
    char *mem;
    size_t capacity;
    size_t offset;
    Arena *next_arena;
}Arena;


void arena_init(size_t capacity, Arena *a);

void *arena_alloc(Arena *a, size_t size);


void arena_reset(Arena *a);

void arena_free(Arena *a);

#endif