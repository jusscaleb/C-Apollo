#include "../headers/variables.h"
#include "../headers/functions.h"
#include "../headers/defs.h"
#include <stdio.h>
#include <string.h>

void register_variable(CodegenContext *context, const char *name,
                       datatype variable_type, FXN *fxn, int level) {
  
  grow_symbols_if_needed(context);

  Symbol *sym = &context->symbols[context->symbol_count++];
  int NAME_LENGTH = strlen(name);

  
  sym->name =  realloc_space(sym->name, NAME_LENGTH);
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

}

void register_fxn(CodegenContext *context, const char *name,
                  datatype return_type, int level) {
  grow_symbols_if_needed(context);
  Symbol *sym = &context->symbols[context->symbol_count++];

  char *new_space = realloc_space(sym->name, strlen(name) + 1);
  sym->name = new_space;
  sym->llvm_name = new_space;
  strcpy(sym->name, name);
  sym->type = return_type;
  sym->str_length = 0;
  sym->t_type = FUNC;
  sym->scope_level = level;
}

Symbol *lookup_token(CodegenContext *context, const char *name, FXN *fxn, int level) {
  for (int i = 0; i < context->symbol_count; i++) {
    if (strcmp(context->symbols[i].name, name) == 0 &&
        strcmp(context->symbols[i].fxn->name , fxn->name) == 0 &&
       strcmp(context->symbols[i].fxn->parent_fxn->name , fxn->parent_fxn->name) == 0 ) {
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


