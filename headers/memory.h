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

typedef struct Arena Arena;

typedef struct Arena{
    char *mem;
    size_t capacity;
    size_t offset;
    Arena *next_arena;
}Arena;

/**
 * Allocates memory initialized to zero.
 * @param num_elements Number of elements to allocate.
 * @param element_size Size of each element.
 * @return A pointer to the allocated memory.
 */
char *alloc_space(int num_elements, int element_size);



void arena_init(size_t capacity, Arena *a);

void *arena_alloc(Arena *a, size_t size);

void arena_reset(Arena *a);

void arena_free(Arena *a);

#endif