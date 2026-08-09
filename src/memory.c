/*=======================================================================================

                         RESOURCE DISTRIBUTOR

=========================================================================================*/

#include "../headers/defs.h"
#include "../headers/error.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../headers/memory.h"

char *realloc_space(char *value, int size) {
  char *alloc_space = realloc(value, size);

  if (alloc_space == NULL) {
    fprintf(stderr, "Memory allocation failed");
    exit(EXIT_FAILURE);
  }
  return alloc_space;
}

char *alloc_space(int num_elements, int element_size) {
  char *alloc_space = calloc(num_elements, element_size);

  if (alloc_space == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return alloc_space;
}

void grow_symbols_if_needed(CodegenContext *context) {
  if (context->symbol_count >= context->symbol_capacity) {

    int new_capacity =
        context->symbol_capacity == 0 ? 8 : context->symbol_capacity * 2;

    Symbol *new_symbols = arena_alloc(context->a, new_capacity * sizeof(Symbol));
        //realloc(context->symbols, new_capacity * sizeof(Symbol));

    //Symbol *new_symbols = arena_alloc(a, new_capacity* sizeof(Symbol));
    
    if (new_symbols == NULL) {
      fprintf(stderr, "FATAL: Failed to allocate memory for symbol table.\n");
      exit(EXIT_FAILURE);
    }

    for (int i = context->symbol_capacity; i < new_capacity; i++) {
      new_symbols[i].name = NULL;
      new_symbols[i].llvm_name = NULL;
    }

    context->symbols = new_symbols;
    context->symbol_capacity = new_capacity;
  }
}

void free_codegen_context(CodegenContext *context) {
  if (context->symbols != NULL) {
    for (int i = 0; i < context->symbol_count; i++) {
      free(context->symbols[i].name);
    }
    free(context->symbols);
    context->symbols = NULL;
    context->symbol_count = 0;
    context->symbol_capacity = 0;
  }

  if (context->deferred_functions != NULL) {
    free(context->deferred_functions);
    context->deferred_functions = NULL;
    context->deferred_count = 0;
    context->deferred_capacity = 0;
  }
}

void codegen_init(CodegenContext *context, const char *output_filename) {
  context->file = NULL;
  context->string_constant_count = 0;
  context->temp_count = 0;
  context->symbol_count = 0;
  context->symbol_capacity = 0;
  context->symbols = NULL;
  context->deferred_functions = NULL;
  context->deferred_count = 0;
  context->deferred_capacity = 0;
}

void realloc_errorStack(ErrorStack *s) {
  if (s->size == s->capacity) {
    s->capacity = (s->capacity == 0) ? 8 : s->capacity * 2;
    s->data = (Error **)realloc(s->data, s->capacity * sizeof(Error *));
    if (s->data == NULL) {
      fprintf(stderr, "FATAL : Failed to realloc memory in error stack");
      exit(EXIT_FAILURE);
    }
  }
}

void errorStack_free(ErrorStack *s) {
  if (s->data != NULL) {
    for (size_t i = 0; i < s->size; i++) {
      free(s->data[i]);
    }
    free(s->data);
    s->data = NULL;
    s->size = 0;
    s->capacity = 0;
  }
}


void arena_init(size_t capacity, Arena *a){
  a->mem = alloc_space(capacity, 1);
  //a->mem = malloc(capacity);

  if(a->mem == NULL){
    perror("Could not allocate memory to arena.");
    exit(1);
  }

  a->capacity = capacity;
  a->offset = 0;
  a->next_arena = NULL;
}


/*void *arena_alloc(Arena *a, size_t size){
  if(a->offset + size > a->capacity){
    a->mem = realloc_space(a->mem, a->capacity * 2);
    a->capacity *= 2;
  }

  void* p = a->mem + a->offset;

  a->offset += size;

  return p;
}

void arena_reset(Arena *a){
  a->offset = 0;

}*/


void* arena_alloc(Arena *a, size_t size){
  size_t  aligned_size = (size + (ARENA_ALIGNMENT - 1)) & ~(ARENA_ALIGNMENT - 1);

  if(a->mem == NULL || (a->offset + aligned_size > a->capacity)){
    size_t new_capacity = a->capacity > 0 ? a->capacity : 1024*1024;

    if(aligned_size > new_capacity) new_capacity = aligned_size;

    
  Arena *old_chunk = (Arena *)alloc_space(1, sizeof(Arena));

  old_chunk->mem = a->mem;
  old_chunk->capacity = a->capacity;
  old_chunk->offset = a->offset;
  old_chunk->next_arena = a->next_arena;

  arena_init(new_capacity, a);
  a->next_arena = old_chunk;

  }

  void *p = a->mem + a->offset;
  a->offset += aligned_size;
  return p;


}

void arena_reset(Arena *a){
  Arena *current = a;

  while(current != NULL){
    current->offset = 0;
    current = current->next_arena;
  }
}

void arena_free(Arena *a) {
  if (a->mem != NULL) {
    free(a->mem);
    a->mem = NULL;
  }
  a->capacity = 0;
  a->offset = 0;
  Arena *curr = a->next_arena;
  while (curr != NULL) {
    Arena *next = curr->next_arena;
    if (curr->mem != NULL) {
      free(curr->mem);
    }
    free(curr);
    curr = next;
  }
  a->next_arena = NULL;
}
