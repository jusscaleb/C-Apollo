#include "../headers/llvm_backend.h"


void _apl_load_runtime_libraries(LLVMComponents *components) {
  const char *libs[] = RUNTIME_LIBS;
  const size_t libs_count = sizeof(libs) / sizeof(libs[0]);

  for (size_t i = 0; i < libs_count; i++) {
    _apl_import_runtime(components->module, libs[i]);
  }
}

void _apl_import_runtime(LLVMModuleRef dest_module, const char *filename) {
  LLVMModuleRef src_module;
  LLVMMemoryBufferRef buffer = NULL;
  char *msg = NULL;

  int success =
      LLVMCreateMemoryBufferWithContentsOfFile(filename, &buffer, &msg);

  if (success != 0) {
    fprintf(stderr, "Error: Could not create memory buffer from file\n");
    LLVMDisposeMessage(msg);
    return;
  }

  int parse_bc = LLVMParseBitcodeInContext2(
      dest_module ? LLVMGetModuleContext(dest_module) : LLVMGetGlobalContext(),
      buffer, &src_module);

  if (parse_bc != 0) {
    fprintf(stderr, "Error parsing bitcode: %s\n", msg);
    LLVMDisposeMessage(msg);
    return;
  }

  int merge_src = LLVMLinkModules2(dest_module, src_module);

  if (merge_src != 0) {
    fprintf(stderr, "Error linking modules!\n");
    if (msg) {
      LLVMDisposeMessage(msg);
    }
    return;
  }

  LLVMDisposeMemoryBuffer(buffer);
  if (msg) {
    LLVMDisposeMessage(msg);
  }
}

void _apl_llvm_environment_setup(CodegenContext *context,
                                 LLVMComponents *components,
                                 ASTNode *block_node) {
  LLVMContextRef ctx = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithNameInContext("apl_module", ctx);
  LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx);

  components->ctx = ctx;
  components->module = module;
  components->builder = builder;

  _apl_load_runtime_libraries(components);
  if (block_node != NULL && block_node->Type == AST_PROGRAM) {
    _apl_gen_block_from_ast(components, context, block_node->program.function);
  } else {
    _apl_gen_block_from_ast(components, context, block_node);
  }

  _apl_save_and_shutdown(components);
}
static LLVMValueRef _apl_get_runtime_function(LLVMComponents *components,
                                              const char *name,
                                              LLVMTypeRef fn_type) {
  LLVMValueRef fn = LLVMGetNamedFunction(components->module, name);
  if (fn != NULL) {
    return fn;
  }
  return LLVMAddFunction(components->module, name, fn_type);
}

void _apl_save_and_shutdown(LLVMComponents *components) {
  char *verify_msg = NULL;
  if (LLVMVerifyModule(components->module, LLVMPrintMessageAction,
                       &verify_msg) != 0) {
    fprintf(stderr, "LLVM module verification failed:\n%s\n",
            verify_msg ? verify_msg : "(no verifier message)");
    if (verify_msg) {
      LLVMDisposeMessage(verify_msg);
    }
    _apl_llvm_shutdown(components);
    exit(EXIT_FAILURE);
  }
  if (verify_msg) {
    LLVMDisposeMessage(verify_msg);
  }

  const int save_err =
      LLVMWriteBitcodeToFile(components->module, "temp\\output.bc");

  if (save_err != 0) {
    fprintf(stderr, "Error: Could not write LLVM IR to file (code %d)\n",
            save_err);
  } else {
    fprintf(stderr, "LLVM IR successfully written to file\n");
  }

  _apl_llvm_shutdown(components);
}

void _apl_llvm_shutdown(LLVMComponents *components) {
  LLVMDisposeModule(components->module);
  LLVMDisposeBuilder(components->builder);
  LLVMContextDispose(components->ctx);
}

// AST Dictionary
void _apl_gen_block_from_ast(LLVMComponents *components,
                             CodegenContext *context, ASTNode *block_node) {
  if (block_node == NULL || block_node->Type != AST_BLOCK)
    return;

  for (int i = 0; i < block_node->block.count; i++) {
    ASTNode *stmt = block_node->block.statements[i];
    if (stmt == NULL)
      continue;

    switch (stmt->Type) {
    case AST_VAR_DECL:
      _apl_create_local_variable(components, context, stmt);
      break;
    case AST_VAR_ASS:
    _apl_reassign_variable(components, context, stmt);
      break;
    case AST_IF:
      // gen_if_from_ast(context, stmt);
      break;

    case AST_WHILE:
      // gen_while_from_ast(context, stmt);
      break;
    case AST_FOR:
      // gen_for_from_ast(context, stmt);
      break;
    case AST_CALL_FXN:
      _apl_gen_fxn_call_from_ast(components, context, stmt);
      break;
    case AST_RET_NODE:
      _apl_gen_function_end(components, context, stmt);

      break;
    case AST_FUNCTION: {
      _apl_gen_function_start(components, context, stmt);
      _apl_gen_block_from_ast(components, context, stmt->function.body);
      _apl_gen_function_end(components, context, stmt);

      break;
    }

    default:
      fprintf(stderr, "Unsupported statement type in block codegen. %d\n",
              stmt->Type);
      _apl_save_and_shutdown(components);
      exit(EXIT_FAILURE);
    }
  }
}

void _apl_gen_fxn_call_from_ast(LLVMComponents *components,
                                CodegenContext *context, ASTNode *stmt) {
  if (memcmp(stmt->call_fxn.name, "println", 7) == 0) {
    _apl_gen_println_ir(components, context, stmt->call_fxn.args);
    return;
  }
}

void _apl_gen_println_ir(LLVMComponents *components, CodegenContext *context,
                         Args *args) {
  DataType arg_type = args->datatype;
  ASTNode *expr = args->arg;
  LLVMTypeRef param_types[1];
  LLVMValueRef println_fxn, println_args;
  LLVMTypeRef func_type;


  switch (arg_type) {
  case TYPE_INT:
  case TYPE_FLOAT: {
    param_types[0] = (arg_type == TYPE_INT)
                         ? LLVMInt32TypeInContext(components->ctx)
                         : LLVMFloatTypeInContext(components->ctx);
    println_fxn = LLVMGetNamedFunction(
        components->module,
        (arg_type == TYPE_INT) ? "_apl_print_int" : "_apl_print_float");

    if(expr->Type == AST_VAR_REF){
      println_args = load_variable(components, expr);
      break;
    }
    if(expr->Type == AST_BINARY_EXPR){
      println_args = arihmetics(components, expr, "");
      break;
    }

    int number;
    float f_number;
    char int_str[expr->literal_expr.token.length + 1];
    memcpy(int_str, expr->literal_expr.token.start,
           expr->literal_expr.token.length + 1);
    int_str[expr->literal_expr.token.length] = '\0';
    if (arg_type == TYPE_INT) {
      number = (int)str_to_int_k(int_str, expr->literal_expr.token.length);
    } else {
      f_number = str_to_int_k(int_str, expr->literal_expr.token.length);
    }



    println_args =
        (arg_type == TYPE_INT)
            ? LLVMConstInt(param_types[0], number, 0)
            : LLVMConstReal(param_types[0], f_number);
    break;
  }
    /*case TYPE_FLOAT:
      println_fxn = LLVMGetNamedFunction(components->module,
      "_apl_print_float"); break;*/

  case TYPE_BOOL:
    int b;
    const int LENGTH = expr->literal_expr.token.length;

    b = (LENGTH == 5) ? 0 : 1;
    param_types[0] = LLVMInt32TypeInContext(components->ctx);
    println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_bool");
    println_args = LLVMConstInt(param_types[0], b, 0);
    break;

  case TYPE_CHAR:
    break;
  case TYPE_STRING: {
    param_types[0] = LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
    println_fxn = LLVMGetNamedFunction(components->module, "_apl_print_string");

    if (expr->Type == AST_VAR_REF) {
      println_args = load_variable(components, expr);
      break;
    }

    char str[expr->literal_expr.token.length + 1];

    slice_string(expr->literal_expr.token, str);
    println_args =
        LLVMBuildGlobalStringPtr(components->builder, str, "println_str");

    break;
  }
  case TYPE_NULL:
      char null_str[4] = "null";

      

    break;

  default:
      println_fxn =
        LLVMGetNamedFunction(components->module, "_apl_print_newline");
    break;
  }

  func_type = LLVMFunctionType(LLVMVoidTypeInContext(components->ctx),
                               param_types, 1, 0);

 
    LLVMBuildCall2(components->builder, func_type, println_fxn, &println_args,
                   1, "");

  if (args->next) {
    _apl_gen_println_ir(components, context, args->next);
  } 

    LLVMValueRef newline_fxn = LLVMGetNamedFunction(components->module, "_apl_print_newline");
    LLVMTypeRef newline_type = LLVMFunctionType(LLVMVoidTypeInContext(components->ctx), NULL, 0, false);
    LLVMBuildCall2(components->builder, newline_type, newline_fxn, NULL, 0, "");
  

  return;
}

void _apl_gen_function_start(LLVMComponents *components,
                             CodegenContext *context, ASTNode *block_node) {
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMTypeRef main_fxn_type = LLVMFunctionType(
        LLVMInt32TypeInContext(components->ctx), NULL, 0, false);
    LLVMValueRef main_fxn =
        LLVMAddFunction(components->module, "main", main_fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, main_fxn, "entry");
    LLVMPositionBuilderAtEnd(components->builder, entry);
    return;
  }

  else {
    LLVMTypeRef fxn_type = LLVMFunctionType(
        _enquire_fxn_return_type(components, &block_node->function.fxn), NULL,
        0, false);
    LLVMValueRef fxn = LLVMAddFunction(components->module,
                                       block_node->function.name, fxn_type);
    LLVMBasicBlockRef entry =
        LLVMAppendBasicBlockInContext(components->ctx, fxn, "entry");
    LLVMPositionBuilderAtEnd(components->builder, entry);
    return;
  }
}

void _apl_gen_function_end(LLVMComponents *components, CodegenContext *context,
                           ASTNode *block_node) {
  if (memcmp(block_node->function.name, "run", 3) == 0) {
    LLVMBuildRet(
        components->builder,
        LLVMConstInt(LLVMInt32TypeInContext(components->ctx), 0, false));
  }
}


LLVMTypeRef _enquire_fxn_return_type(LLVMComponents *components, Fxn *fxn) {
  switch (fxn->return_type) {
  case TYPE_INT:
    return LLVMInt32TypeInContext(components->ctx);
  case TYPE_FLOAT:
    return LLVMFloatTypeInContext(components->ctx);
  case TYPE_STRING:
    return LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0);
  default:
    return LLVMVoidTypeInContext(components->ctx);
  }
}

void _apl_create_local_variable(LLVMComponents *components,
                                CodegenContext *context, ASTNode *var_node) {

  char var_name[var_node->var_decl.name_length + 1];
  memcpy(var_name, var_node->var_decl.name, var_node->var_decl.name_length);
  var_name[var_node->var_decl.name_length] = '\0';

  ASTNode *expr = var_node->var_decl.value;
  const DataType VAR_TYPE = var_node->var_decl.value_type;

  LLVMValueRef var_ptr;
  LLVMValueRef assign_val;
  LLVMValueRef var_val;

  switch (VAR_TYPE) {

  case TYPE_BOOL:{
    const int LENGTH = expr->literal_expr.token.length;

    int b_val = (LENGTH == 5) ? 0 : 1;

    var_ptr = LLVMBuildAlloca(components->builder, LLVMInt1TypeInContext(components->ctx), var_name);

    LLVMBuildStore(components->builder, LLVMConstInt(LLVMInt1TypeInContext(components->ctx), b_val, 0), var_ptr );

    break;
  }
  case TYPE_FLOAT:{
    LLVMValueRef val;

    if(expr->Type == AST_LITERAL_EXPR){
    const int LENGTH = expr->literal_expr.token.length;

    char f_str[LENGTH + 1];
    memcpy(f_str, expr->literal_expr.token.start, LENGTH);
    f_str[LENGTH] = '\0';

    float f_val = str_to_int_k(f_str, LENGTH);
    val = LLVMConstReal(LLVMFloatTypeInContext(components->ctx), f_val);
    }
    else{
      val = arihmetics(components, expr, "");

    }

    var_ptr = LLVMBuildAlloca(components->builder, LLVMFloatTypeInContext(components->ctx), var_name);
    LLVMBuildStore(components->builder, val, var_ptr);


    break;
  }
  case TYPE_INT:{
    LLVMValueRef val;
    if(expr->Type == AST_LITERAL_EXPR){
    const int LENGTH = expr->literal_expr.token.length;

    char i_str[LENGTH + 1];
    memcpy(i_str, expr->literal_expr.token.start, LENGTH);
    i_str[LENGTH] = '\0';
    int i_val = (int) str_to_int_k(i_str, LENGTH);
    val = LLVMConstInt(LLVMInt32TypeInContext(components->ctx), i_val ,0);
    }else{
      val = arihmetics(components, expr , "");
    }

    var_ptr = LLVMBuildAlloca(components->builder, LLVMInt32TypeInContext(components->ctx), var_name);
    LLVMBuildStore(components->builder, val , var_ptr);
    break;

  }
  
  case TYPE_STRING:{
    char str[expr->literal_expr.token.length + 1];

    slice_string(expr->literal_expr.token, str);
    LLVMTypeRef str_members[] = {
      LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
      LLVMInt32TypeInContext(components->ctx)
    };

    LLVMTypeRef string_struct_type = LLVMStructTypeInContext(components->ctx, str_members, 2,false);
    var_ptr = LLVMBuildAlloca(components->builder, string_struct_type, var_name);
    LLVMTypeRef param_types[] = {LLVMPointerType(string_struct_type, 0), LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0)};
    LLVMTypeRef create_str_fxn_type = LLVMFunctionType(LLVMVoidTypeInContext(components->ctx), param_types, 2, false);
    LLVMValueRef create_str_fxn = LLVMGetNamedFunction(components->module, "__apl_create_str__");
    LLVMValueRef llvm_str_lit = LLVMBuildGlobalStringPtr(components->builder, str, "str_lit");
    LLVMValueRef args[] = { var_ptr, llvm_str_lit };

    LLVMValueRef ret_struct = LLVMBuildCall2(components->builder, create_str_fxn_type, create_str_fxn, args, 2, "");

    
    break;
  }
  default:
    break;
  }
  var_node->var_decl.resolved_symbol->llvm_val_ref = var_ptr;
}

//===================================================HELPERS======================================================>
__attribute__((always_inline)) LLVMValueRef load_variable(LLVMComponents *components, ASTNode *var_ref_node){
  Symbol* var_sym = var_ref_node->var_ref.resolved_symbol;
  LLVMValueRef llvm_var;

  switch(var_sym->type){
    case TYPE_INT:{
      llvm_var = LLVMBuildLoad2(components->builder, LLVMInt32TypeInContext(components->ctx), var_sym->llvm_val_ref, var_sym->name);
      break;
    }
    case TYPE_STRING: {
      LLVMTypeRef str_members[] = {
        LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0),
        LLVMInt32TypeInContext(components->ctx)
      };
      LLVMTypeRef string_struct_type = LLVMStructTypeInContext(components->ctx, str_members, 2, false);
      LLVMValueRef char_ptr_gep = LLVMBuildStructGEP2(components->builder, string_struct_type, var_sym->llvm_val_ref, 0, "str_gep");
      llvm_var = LLVMBuildLoad2(components->builder, LLVMPointerType(LLVMInt8TypeInContext(components->ctx), 0), char_ptr_gep, var_sym->name);
      break;
    }
    
    case TYPE_FLOAT: {
      llvm_var = LLVMBuildLoad2(components->builder,LLVMFloatTypeInContext(components->ctx), var_sym->llvm_val_ref, var_sym->name);
      break;
  
    }
    case TYPE_BOOL:{
      break;

    }
    
  }

  return llvm_var;



}


__attribute__((always_inline)) void slice_string(Token string, char *clean_str) {
  const char *sliced_start = string.start + 1;

  int sliced_length = string.length - 1;
  memcpy(clean_str, sliced_start, sliced_length);

  clean_str[sliced_length] = '\0';
}

float str_to_int_k(const char *s, int k) {
  int result = 0;
  float final;
  int dp = 1;
  bool is_decimal_place = false;
  bool negative = false;

  for (int i = 0; i < k; i++) {
    if (s[i] == '-') {
      negative = true;
      continue;
    }
    if (s[i] == '.') {
      is_decimal_place = true;
      continue;
    }
    result = result * 10 + (s[i] - '0');

    if (is_decimal_place)
      dp *= 10;
  }

  final = (float)result / dp;

  return (negative) ? final * -1 : final;
}

LLVMValueRef arihmetics(LLVMComponents *components, ASTNode *node, char *result_name) {
  if (!node) return NULL;

  if (node->Type == AST_LITERAL_EXPR) {
    int val = (int)str_to_int_k(node->literal_expr.token.start, node->literal_expr.token.length);
    return LLVMConstInt(LLVMInt32TypeInContext(components->ctx), val, 0);
  }

  if (node->Type == AST_VAR_REF) {
    return load_variable(components, node);
  }

  if (node->Type == AST_BINARY_EXPR) {
    LLVMValueRef left = arihmetics(components, node->binary_expr.left, "left_tmp");
    LLVMValueRef right = arihmetics(components, node->binary_expr.right, "right_tmp");

    switch (node->binary_expr.operator_type) {
      case TOKEN_ADD:
        return LLVMBuildAdd(components->builder, left, right, result_name);
      case TOKEN_SUB:
        return LLVMBuildSub(components->builder, left, right, result_name);
      case TOKEN_DIV:
        return LLVMBuildSDiv(components->builder, left, right, result_name);
      case TOKEN_MUL:
        return LLVMBuildMul(components->builder, left, right, result_name);
      default:
        return NULL;
    }
  }

  return NULL;
}

void _apl_reassign_variable(LLVMComponents *components, CodegenContext *context, ASTNode *node){
  
  DataType var_Type = node->var_assign.resolved_symbol->type;

  LLVMValueRef var = node->var_assign.resolved_symbol->llvm_val_ref;
  LLVMValueRef new_val;

  Token literal_expr = node->var_assign.value->literal_expr.token;
  char val[literal_expr.length + 1];

  if(node->Type == AST_VAR_ASS){
  memcpy(val, literal_expr.start , literal_expr.length);
  val[literal_expr.length] = '\0';

  }

  switch(var_Type){
    case TYPE_INT: {
      int i_val = (int) str_to_int_k(val, literal_expr.length);
      new_val = LLVMConstInt(LLVMInt32TypeInContext(components->ctx), i_val , 0);
      break;
    }

    case TYPE_FLOAT:{
      break;
    }
    case TYPE_STRING: {

      break;

    }

    case TYPE_BOOL:{

      break;
    }
    


    default:
      break;
  }

  LLVMBuildStore(components->builder, new_val, var);
}
