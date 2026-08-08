#include "../headers/variables.h"
#include "../headers/defs.h"
#include "../headers/functions.h"
#include <string.h>


Symbol *register_variable(CodegenContext *context, const char *name,
                          DataType variable_type, Fxn *fxn, int level) {

  grow_symbols_if_needed(context);

  Symbol *sym = &context->symbols[context->symbol_count++];
  int NAME_LENGTH = strlen(name);

  sym->name = realloc_space(sym->name, NAME_LENGTH);
  sprintf(sym->name, "%s", name);
  sym->llvm_name = realloc_space(sym->llvm_name, NAME_LENGTH);

  sprintf(sym->llvm_name, "%s", sym->name);
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

  char *new_space = realloc_space(sym->name, strlen(name) + 1);
  sym->name = new_space;

  strcpy(sym->name, name);

  if (strcmp(name, "run") == 0) {
    sym->llvm_name = realloc_space(sym->llvm_name, strlen(name) + 1);
    strcpy(sym->llvm_name, name);
  } else {
    // Mangle the name to ensure uniqueness across nested functions
    char mangled[256];
    snprintf(mangled, sizeof(mangled), "%s_%d", name, context->symbol_count);
    sym->llvm_name = realloc_space(sym->llvm_name, strlen(mangled) + 1);
    strcpy(sym->llvm_name, mangled);
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
