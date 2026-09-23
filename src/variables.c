#include "../headers/variables.h"
#include "../headers/defs.h"
#include "../headers/functions.h"
#include "../headers/memory.h"
#include <stdint.h>

const char *dt_names[] = {"str", "int", "char", "float", "bool", "null"};

Symbol *register_variable(CodegenContext *context, const char *name,
                          DataType variable_type, Fxn *fxn, int level, int len) {

  grow_symbols_if_needed(context);

  Symbol *sym = &context->symbols[context->symbol_count++];
  int NAME_LENGTH = len;

  sym->name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);
  memcpy(sym->name, name, NAME_LENGTH);
  sym->name[NAME_LENGTH] = '\0';
  sym->llvm_name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);

  memcpy(sym->llvm_name, sym->name, NAME_LENGTH);
  sym->llvm_name[NAME_LENGTH] = '\0';
  sym->type = variable_type;

  (variable_type == TYPE_STRING)
      ? sym->str_const_id = context->string_constant_count++
      : 0;

  sym->t_type = VAR;
  sym->scope_level = level;
  sym->name_length = NAME_LENGTH;
  sym->fxn = fxn;
  sym->is_active = true;
  sym->frame_index = context->symbol_count - 1;

  if (context && context->t) {
    symbol_table_insert(context->t, sym, context->a);
  }

  return sym;
}

Symbol *register_fxn(CodegenContext *context, const char *name,
                     DataType return_type, int level, Fxn *fxn, int len) {
  grow_symbols_if_needed(context);
  Symbol *sym = &context->symbols[context->symbol_count++];

  const uint32_t NAME_LENGTH = len;

  sym->name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);
  memcpy(sym->name, name, NAME_LENGTH);

  sym->name[NAME_LENGTH] = '\0';

  if (memcmp(name, "run", 3) == 0) {
    sym->llvm_name = (char *)arena_alloc(context->a, NAME_LENGTH + 1);

    memcpy(sym->llvm_name, name, NAME_LENGTH);
    sym->llvm_name[NAME_LENGTH] = '\0';
  } else {
    // get number of digits.
    uint32_t s_count = (uint32_t)context->symbol_count;
    uint32_t n_digits = 0;
    while (s_count != 0) {
      n_digits++;
      s_count /= 10;
    }

    char mangled[256];

    memcpy(mangled, name, NAME_LENGTH);
    mangled[NAME_LENGTH] = '_';

    uint32_t i = NAME_LENGTH + 1 + n_digits;

    mangled[i] = '\0';

    uint32_t s_count_2 = (uint32_t)context->symbol_count;

    do {
      mangled[--i] = '0' + (s_count_2 % 10);
      s_count_2 /= 10;
    } while (s_count_2 > 0);

    sym->llvm_name = arena_alloc(context->a, i + 2);
    memcpy(sym->llvm_name, mangled, i);

    sym->llvm_name[i] = '\0';
  }

  sym->type = return_type;
  sym->str_const_id = 0;
  sym->name_length = NAME_LENGTH;
  sym->t_type = FUNC;
  sym->scope_level = level;
  sym->fxn = fxn;
  sym->bucket = fxn ? fxn->bucket : BUCKET_PLUS_ONE;
  sym->is_active = true;

  if (context && context->t) {
    symbol_table_insert(context->t, sym, context->a);
  }

  return sym;
}

Symbol *lookup_token(CodegenContext *context, const char *name, Fxn *fxn,
                     int level) {
  if (context && context->t) {
    return symbol_table_lookup(context->t, name, strlen(name));
  }
  return NULL;
}

Symbol *get_token(CodegenContext *context, const char *name) {
  for (int i = 0; i < context->symbol_count; i++) {
    if (strcmp(context->symbols[i].name, name) == 0) {
      return &context->symbols[i];
    }
  }
  return NULL;
}

SymbolTable *symbol_table_init(Arena *a, uint32_t initial_capacity) {

  SymbolTable *t = (SymbolTable *)arena_alloc(a, sizeof(SymbolTable));
  t->capacity = initial_capacity > 0 ? initial_capacity : 16;

  t->count = 0;
  t->buckets =
t->buckets = (SymbolEntry **)arena_alloc(a, sizeof(SymbolEntry*) * t->capacity);

  memset(t->buckets, 0, sizeof(SymbolEntry *) * t->capacity);

  return t;
}

void symbol_table_resize(SymbolTable *table, Arena *a) {
  if (table->count < (uint32_t)(table->capacity * 0.75)) {
    return;
  }

  uint32_t old_capacity = table->capacity;

  SymbolEntry **old_buckets = table->buckets;

  table->capacity *= 2;

  table->buckets =
      (SymbolEntry **)arena_alloc(a, sizeof(SymbolEntry *) * table->capacity);

  memset(table->buckets, 0, sizeof(SymbolEntry *) * table->capacity);

  for (uint32_t i = 0; i < old_capacity; i++) {
    SymbolEntry *entry = old_buckets[i];

    while (entry != NULL) {
      SymbolEntry *next = entry->next;

      uint32_t new_index = entry->hash & (table->capacity - 1);

      entry->next = table->buckets[new_index];
      table->buckets[new_index] = entry;

      entry = next;
    }
  }
}

ALWAYS_INLINE uint32_t hash_string(const char *key,
                                                    uint32_t length) {
  uint32_t hash = 2166136261u;

  for (uint32_t i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619u;
  }

  return hash;
}

void symbol_table_insert(SymbolTable *table, Symbol *sym, Arena *a) {
  symbol_table_resize(table, a);
  uint32_t len = sym->name_length;
  uint32_t hash = hash_string(sym->name, len);
  uint32_t index = hash & (table->capacity - 1);

  SymbolEntry *e = (SymbolEntry *)arena_alloc(a, sizeof(SymbolEntry));

  e->sym = sym;
  e->hash = hash;
  e->next = table->buckets[index];
  table->buckets[index] = e;

  table->count++;
}

Symbol *symbol_table_lookup(SymbolTable *table, const char *name, uint32_t name_length) {
    if (!table || !table->buckets || table->capacity == 0) return NULL;

    uint32_t hash = hash_string(name, name_length);
    uint32_t index = hash & (table->capacity - 1);
    
    SymbolEntry *entry = table->buckets[index];
    while (entry != NULL) {
        Symbol *sym = entry->sym;
        
        if (sym && sym->is_active) {
            if (sym->name_length == name_length) {
                if (memcmp(sym->name, name, name_length) == 0) {
                    return sym; 
                }
            }
        }
        entry = entry->next;
    }
    return NULL;
}

