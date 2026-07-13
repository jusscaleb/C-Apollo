/*--------------------------------------------------------------------------------

                         RESOURCE DISTRIBUTOR

---------------------------------------------------------------------------------*/

#include "../headers/defs.h"
#include "../headers/error.h"
#include <stdio.h>
#include <stdlib.h>

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

    int new_capacity = context->symbol_capacity == 0 ? 8 : context->symbol_capacity * 2;

    Symbol *new_symbols = realloc(context->symbols, new_capacity * sizeof(Symbol));
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

void free_codegen_context(CodegenContext *context, ErrorStack *s) {
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
