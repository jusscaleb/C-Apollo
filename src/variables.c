#include "../headers/variables.h"
#include "../headers/defs.h"
#include "../headers/functions.h"
#include <stdint.h>
#include <string.h>


Symbol *register_variable(CodegenContext *context, const char *name,
                          DataType variable_type, Fxn *fxn, int level) {

  grow_symbols_if_needed(context);

  Symbol *sym = &context->symbols[context->symbol_count++];
  int NAME_LENGTH = strlen(name);

  sym->name = (char*)arena_alloc(context->a, NAME_LENGTH+1);
  memcpy(sym->name, name , NAME_LENGTH);
  sym->name[NAME_LENGTH] = '\0';
  sym->llvm_name = (char*) arena_alloc(context->a, NAME_LENGTH+1);
  
  memcpy(sym->llvm_name, sym->name, NAME_LENGTH);
  sym->llvm_name[NAME_LENGTH] = '\0';
  sym->type = variable_type;

  (variable_type == TYPE_STRING)
      ? sym->str_length = context->string_constant_count++
      : 0;

  sym->t_type = VAR;
  sym->scope_level = level;
  sym->fxn = fxn;
  sym->is_active = true;

  return sym;
}

Symbol *register_fxn(CodegenContext *context, const char *name,
                     DataType return_type, int level, Fxn *fxn) {
  grow_symbols_if_needed(context);
  Symbol *sym = &context->symbols[context->symbol_count++];

  const uint32_t NAME_LENGTH = strlen(name);

  sym->name = (char*)arena_alloc(context->a, NAME_LENGTH+1);
  memcpy(sym->name, name, NAME_LENGTH);

  sym->name[NAME_LENGTH] = '\0';

  if (memcmp(name, "run", 3) == 0) {
    sym->llvm_name = (char*) arena_alloc(context->a, NAME_LENGTH+1);

    memcpy(sym->llvm_name, name, NAME_LENGTH);
    sym->llvm_name[NAME_LENGTH] = '\0';
  } else {
    //get number of digits.
    uint32_t s_count = (uint32_t) context->symbol_count;
    uint32_t n_digits = 0;
    while(s_count != 0){
      n_digits++;
      s_count %= 10;
      s_count /=10;
    }

    char mangled[NAME_LENGTH + n_digits + 2];


   memcpy(mangled, name, NAME_LENGTH);
   mangled[NAME_LENGTH] = '_';

   uint32_t i = NAME_LENGTH + 1 + n_digits;

   mangled[i] = '\0';

   uint32_t s_count_2 = (uint32_t) context->symbol_count;

   do {
    mangled[--i] = '0' + (s_count_2 % 10);
   }while(s_count_2 > 0);

    sym->llvm_name = arena_alloc(context->a, i+2);
    memcpy(sym->llvm_name, mangled, i);

    sym->llvm_name[i] = '\0';
  }

  sym->type = return_type;
  sym->str_length = 0;
  sym->t_type = FUNC;
  sym->scope_level = level;
  sym->fxn = fxn;
  sym->is_active = true;
  return sym;
}

Symbol *lookup_token(CodegenContext *context, const char *name, Fxn *fxn,
                     int level) {
  for (int i = context->symbol_count - 1; i >= 0; i--) {
    if (strcmp(context->symbols[i].name, name) != 0)
      continue;
    if (context->symbols[i].is_active) {
      return &context->symbols[i];
    }
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
