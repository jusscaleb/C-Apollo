/*===================================================================
                          memory.h

                        (c)2026 SCXRPIUS.dev

              The Apollo Memory Compile time library.
          Developed by Caleb Dhliwayo (calebbrandon999@gmail.com)
     
----------------------------------------------------------------------
    Licensed under the MIT License. See LICENSE file for details.
====================================================================*/

#include "../headers/defs.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MEMORY_H
#define MEMORY_H

#define ARENA_ALIGNMENT 8

typedef struct Arena{
    char *mem;
    size_t capacity;
    size_t offset;
    struct Arena *next_arena;
} Arena;

/**
 * Allocates memory initialized to zero.
 * @param num_elements Number of elements to allocate.
 * @param element_size Size of each element.
 * @return A pointer to the allocated memory.
 */
char *alloc_space(size_t num_elements, size_t element_size);



/**
 * Initializes an arena allocator memory block.
 * @param capacity Maximum capacity in bytes for the arena.
 * @param a Pointer to the arena struct to initialize.
 */
void arena_init(size_t capacity, Arena *a);

/**
 * Allocates a contiguous block of memory from the arena.
 * @param a Pointer to the arena allocator.
 * @param size Number of bytes to allocate.
 * @return Pointer to the allocated memory block.
 */
void *arena_alloc(Arena *a, size_t size);

/**
 * Resets the arena allocation offset back to zero without freeing memory block.
 * @param a Pointer to the arena allocator.
 */
void arena_reset(Arena *a);

/**
 * Frees all memory regions owned by the arena allocator.
 * @param a Pointer to the arena allocator.
 */
void arena_free(Arena *a);

#endif