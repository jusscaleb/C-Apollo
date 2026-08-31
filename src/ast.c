#include "../headers/ast.h"
#include <stdint.h>


static ASTNode *allocate_node(ASTNodeType type, Arena *a) {
  ASTNode *node = arena_alloc(a, sizeof(ASTNode));
  if (node == NULL) {
    fprintf(stderr, "CRITICAL: Could not allocate AST node.\n");
    exit(EXIT_FAILURE);
  }

  node->Type = type;

  return node;
}

ASTNode *create_literal_node(Token token, Arena *a) {
  ASTNode *node = allocate_node(AST_LITERAL_EXPR, a);
  node->literal_expr.token = token;
  node->literal_expr.line = token.line;
  node->literal_expr.column = token.column;

  return node;
}



ASTNode *create_binary_node(ASTNode *left, TokenType operator_type,
                            ASTNode *right, Arena *a) {
  ASTNode *node = allocate_node(AST_BINARY_EXPR, a);

  node->binary_expr.left = left;
  node->binary_expr.operator_type = operator_type;
  node->binary_expr.right = right;
  node->binary_expr.line = line;
  node->binary_expr.column = column;

  return node;
}

ASTNode *create_var_decl_node(const char *name, int name_length,
                              DataType value_type, ASTNode *value, Fxn fxn,
                              int level, Arena *a, int pointer_level, MemoryBucket bucket,
                              int array_count) {
  ASTNode *node = allocate_node(AST_VAR_DECL, a);

  node->var_decl.name = name;
  node->var_decl.name_length = name_length;
  node->var_decl.value_type = value_type;
  node->var_decl.value = value;
  node->var_decl.fxn = fxn;
  node->var_decl.pointer_level = pointer_level;
  node->var_decl.level = level;
  node->var_decl.bucket = bucket;
  node->var_decl.array_count = array_count;

  return node;
}

ASTNode *create_var_ref_node(const char *name, int name_length, Fxn fxn,
                             int level, Arena *a) {
  ASTNode *node = allocate_node(AST_VAR_REF, a);
  node->var_ref.name = name;
  node->var_ref.name_length = name_length;
  node->var_ref.fxn = fxn;
  node->var_ref.level = level;
  node->var_ref.line = line;
  node->var_ref.column = column;

  return node;
}

ASTNode *create_var_assign_node(const char *name, int name_length,
                                ASTNode *value, Fxn fxn, int level, Arena *a, int deref_level) {
  ASTNode *node = allocate_node(AST_VAR_ASS, a);
  node->var_assign.name = name;
  node->var_assign.name_length = name_length;
  node->var_assign.value = value;
  node->var_assign.fxn = fxn;
  node->var_assign.level = level;
  node->var_assign.deref_level = deref_level;
  node->var_assign.target_node = NULL;
  return node;
}


ASTNode *create_if_node(ASTNode *condition, ASTNode *then_block,
                        ASTNode *else_block, Arena *a) {
  ASTNode *node = allocate_node(AST_IF, a);
  node->if_stmt.condition = condition;
  node->if_stmt.then_block = then_block;
  node->if_stmt.else_block = else_block;
  node->if_stmt.line = line;
  node->if_stmt.column = column;
  return node;
}

ASTNode *create_while_node(ASTNode *condition, ASTNode *then_block, Arena *a) {
  ASTNode *node = allocate_node(AST_WHILE, a);
  node->while_lp.condition = condition;
  node->while_lp.then_block = then_block;
  node->while_lp.line = line;
  node->while_lp.column = column;

  return node;
}

ASTNode *create_block_node(int level, Arena *a) {
  ASTNode *node = allocate_node(AST_BLOCK, a);
  node->block.count = 0;
  node->block.capacity = 8;
  node->block.level = level;
  node->block.statements = arena_alloc(a,sizeof(ASTNode *) * node->block.capacity ) ;//malloc(sizeof(ASTNode *) * node->block.capacity);
  return node;
}

ASTNode *create_for_node(ASTNode *variable, ASTNode *condition,
                         ASTNode *var_operation, ASTNode *then_block, Arena *a) {
  ASTNode *node = allocate_node(AST_FOR, a);
  node->for_lp.var_operation = var_operation;
  node->for_lp.condtion = condition;
  node->for_lp.variable = variable;
  node->for_lp.then_block = then_block;
  node->for_lp.line = line;
  node->for_lp.column = column;
  return node;
}

void block_add_statement(ASTNode *block, ASTNode *statement, Arena *a) {
  if (block->block.count >= block->block.capacity) {
    block->block.capacity *= 2;
    block->block.statements = realloc(
        block->block.statements, sizeof(ASTNode *) * block->block.capacity);
  }
  block->block.statements[block->block.count++] = statement;
}

ASTNode *create_fxn_call_node(char *name, int name_length,
                              Fxn fxn, int level, Args *args, Arena *a) {
  
  ASTNode *node = allocate_node(AST_CALL_FXN, a);
  

  printf("CALLING NODE: %s\n", name);
  node->call_fxn.name = name;
  node->call_fxn.name_length = name_length;
  node->call_fxn.fxn = fxn;
  node->call_fxn.level = level;
  node->call_fxn.args = args ? args : NULL;

  return node;
}

ASTNode *create_function_node(const char *name, int name_length, ASTNode *body,
                              Fxn fxn, int level, Arena *a, int pointer_level) {
  ASTNode *node = allocate_node(AST_FUNCTION, a);
  node->function.name = name;
  node->function.name_length = name_length;
  node->function.body = body;
  node->function.fxn = fxn;
  node->function.level = level;
  node->function.pointer_level = pointer_level;
  
  return node;
}

ASTNode *create_program_node(ASTNode *block, Arena *a) {
  ASTNode *node = allocate_node(AST_PROGRAM, a);
  node->program.function =
      block; // Using 'function' field to store the block of functions for now
  if (block) {
    node->program.line = block->block.line;
    node->program.column = block->block.column;
  }
  return node;
}

ASTNode *create_ret_node(CodegenContext *context, Fxn fxn, ASTNode *value, Arena *a) {
  ASTNode *node = allocate_node(AST_RET_NODE, a);
  node->ret_node.fxn = fxn;
  node->ret_node.value = value;
  node->ret_node.line = line;
  node->ret_node.column = column;
  return node;
}

ASTNode *create_urinary_node(TokenType operator_type, ASTNode *value, Arena *a){
  ASTNode *node = allocate_node(AST_URINARY_EXPR, a);
  node->urinary_expr.operator_type = operator_type;
  node->urinary_expr.value = value;
  return node;
}

ASTNode *create_array_node(ASTNode **elements, uint32_t count, uint32_t capacity, bool is_dynamic, DataType element_type,  Arena *a){
  ASTNode *node = allocate_node(AST_ARRAY_LITERAL, a);
  node->array_literal.elements = elements;
  node->array_literal.is_dynamic = is_dynamic;
  node->array_literal.count = count;
  node->array_literal.capacity = capacity;
  node->array_literal.element_type = element_type;
  return node;
}

ASTNode *create_index_expr_node(ASTNode *target, ASTNode *index, DataType datatype, Arena *a){
  ASTNode *node = allocate_node(AST_INDEX_EXPR, a);
  node->index_expr.target = target;
  node->index_expr.index = index;
  node->index_expr.type = datatype;
  return node;  
}

ASTNode *create_struct_node(Struct *fields, Fxn *fxn, int level, Arena *a, char* name, int name_length){
  ASTNode *node = allocate_node(AST_STRUCT_DEFINITION, a);
  node->struct_expr.fxn = fxn;
  node->struct_expr.fields = fields;
  node->struct_expr.level = level;
  node->struct_expr.name = name;
  node->struct_expr.name_length = name_length;
  return node;
}

ASTNode *create_access_node(ASTNode *src, ASTNode *target, DataType dt, Arena *a){
  ASTNode *node = allocate_node(AST_ACCESS, a);
  node->access.datatype = dt;
  node->access.src = src;
  node->access.target = target;
  node->access.struct_type_name = NULL;
  node->access.fields = NULL;

  return node;
}

ASTNode *create_target_assign_node(ASTNode *target_node, ASTNode *value, Fxn fxn, int level, Arena *a) {
  ASTNode *node = allocate_node(AST_VAR_ASS, a);
  node->var_assign.name = NULL;
  node->var_assign.name_length = 0;
  node->var_assign.value = value;
  node->var_assign.fxn = fxn;
  node->var_assign.level = level;
  node->var_assign.deref_level = 0;
  node->var_assign.resolved_symbol = NULL;
  node->var_assign.target_node = target_node;
  return node;
}

ASTNode *create_fxns_node(char *name, int name_length, int level, Fxns *fxns, Arena *a) {
  ASTNode *node = allocate_node(AST_FUNCTIONS, a);
  node->functions.name = name;
  node->functions.name_length = name_length;
  node->functions.level = level;
  node->functions.count = 0;
  node->functions.capacity = 4;
  node->functions.methods = (ASTNode **)arena_alloc(a, sizeof(ASTNode *) * node->functions.capacity);
  node->functions.fxns = fxns;
  node->functions.resolved_symbol = NULL;
  return node;
}

void fxns_add_method(ASTNode *node, ASTNode *method, Arena *a) {
  if (!node || node->Type != AST_FUNCTIONS || !method)
    return;

  if (node->functions.count >= node->functions.capacity) {
    node->functions.capacity *= 2;
    ASTNode **new_methods = (ASTNode **)arena_alloc(a, sizeof(ASTNode *) * node->functions.capacity);
    memcpy(new_methods, node->functions.methods, sizeof(ASTNode *) * node->functions.count);
    node->functions.methods = new_methods;
  }
  node->functions.methods[node->functions.count++] = method;
}

