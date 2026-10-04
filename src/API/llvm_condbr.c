#include "../../headers/llvm_backend.h"


void _apl_gen_if_block(LLVMComponents *components,
                       ASTNode *node) {
  ASTNode *cond = node->if_stmt.condition;
  ASTNode *then = node->if_stmt.then_block;
  ASTNode *elseB = node->if_stmt.else_block;
  LLVMValueRef cond_gen;

  if (cond->Type == AST_LITERAL_EXPR &&
      cond->literal_expr.token.type != TYPE_NULL)
    cond_gen = LLVMConstInt(I1(components->ctx), 1, 0);
  else if (cond->Type == AST_BINARY_EXPR)
    cond_gen = arihmetics(components, cond, "");
  else
    cond_gen = LLVMConstInt(I1(components->ctx), 0, 0);

  LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(components->current_fxn, "");
  LLVMBasicBlockRef elseBB =
      (elseB) ? LLVMAppendBasicBlock(components->current_fxn, "") : NULL;
  LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(components->current_fxn, "");

  LLVMBuildCondBr(components->builder, cond_gen, thenBB,
                  elseBB ? elseBB : mergeBB);

  LLVMPositionBuilderAtEnd(components->builder, thenBB);

  _apl_gen_block_from_ast(components, then);

  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(components->builder))) {
    LLVMBuildBr(components->builder, mergeBB);
  }

  if (elseBB) {
    LLVMPositionBuilderAtEnd(components->builder, elseBB);
    if (elseB->Type == AST_IF) {
      _apl_gen_if_block(components, elseB);
    } else {
      _apl_gen_block_from_ast(components, elseB);
    }
    if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(components->builder))) {
      LLVMBuildBr(components->builder, mergeBB);
    }
  }

  LLVMPositionBuilderAtEnd(components->builder, mergeBB);
}

void _apl_gen_while_loop(LLVMComponents *components,
                         ASTNode *node) {
  ASTNode *cond = node->while_lp.condition;
  ASTNode *then = node->while_lp.then_block;

  LLVMBasicBlockRef condBB =
      LLVMAppendBasicBlock(components->current_fxn, "while_cond");
  LLVMBasicBlockRef thenBB =
      LLVMAppendBasicBlock(components->current_fxn, "while_body");
  LLVMBasicBlockRef afterBB =
      LLVMAppendBasicBlock(components->current_fxn, "while_end");

  LLVMBuildBr(components->builder, condBB);

  LLVMPositionBuilderAtEnd(components->builder, condBB);
  LLVMValueRef cond_gen;
  if (cond->Type == AST_BINARY_EXPR)
    cond_gen = arihmetics(components, cond, "");
  else if (cond->Type == AST_LITERAL_EXPR &&
           cond->literal_expr.token.type != TOKEN_NULL)
    cond_gen = LLVMConstInt(I1(components->ctx), 1, 0);
  else
    cond_gen = LLVMConstInt(I1(components->ctx), 0, 0);
  LLVMBuildCondBr(components->builder, cond_gen, thenBB, afterBB);

  LLVMPositionBuilderAtEnd(components->builder, thenBB);
  LLVMValueRef mark_val = _apl_emit_arena_get_mark(components, components->current_arena_ptr);
  _apl_gen_block_from_ast(components, then);
  _apl_emit_arena_set_mark(components, components->current_arena_ptr, mark_val);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(components->builder))) {
    LLVMBuildBr(components->builder, condBB);
  }

  LLVMPositionBuilderAtEnd(components->builder, afterBB);
}

void _apl_gen_for_loop(LLVMComponents *components,
                       ASTNode *node) {
  ASTNode *init = node->for_lp.variable;
  ASTNode *cond = node->for_lp.condtion;
  ASTNode *step = node->for_lp.var_operation;
  ASTNode *body = node->for_lp.then_block;

  if (init != NULL) {
    if (init->Type == AST_VAR_DECL) {
      _apl_create_local_variable(components, init);
    } else if (init->Type == AST_VAR_ASS) {
      _apl_reassign_variable(components, init);
    }
  }

  LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(components->current_fxn, "");
  LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(components->current_fxn, "");
  LLVMBasicBlockRef stepBB = LLVMAppendBasicBlock(components->current_fxn, "");
  LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(components->current_fxn, "");

  LLVMBuildBr(components->builder, condBB);

  LLVMPositionBuilderAtEnd(components->builder, condBB);
  LLVMValueRef cond_gen;
  if (cond != NULL) {
    if (cond->Type == AST_BINARY_EXPR) {
      cond_gen = arihmetics(components, cond, "");
    } else if (cond->Type == AST_LITERAL_EXPR &&
               cond->literal_expr.token.type != TOKEN_NULL) {
      cond_gen = LLVMConstInt(I1(components->ctx), 1, 0);
    } else if (cond->Type == AST_VAR_REF) {
      cond_gen = load_variable(components, cond);
    } else {
      cond_gen = LLVMConstInt(I1(components->ctx), 1, 0);
    }
  } else {
    cond_gen = LLVMConstInt(I1(components->ctx), 1, 0);
  }
  LLVMBuildCondBr(components->builder, cond_gen, bodyBB, afterBB);

  LLVMPositionBuilderAtEnd(components->builder, bodyBB);
  LLVMValueRef mark_val = _apl_emit_arena_get_mark(components, components->current_arena_ptr);
  if (body != NULL) {
    switch (body->Type){
      case AST_BLOCK:{
        _apl_gen_block_from_ast(components, body);
        break;
      }
      case AST_VAR_DECL:{
        _apl_create_local_variable(components, body);
        break;
      }

      case AST_VAR_ASS: {
          _apl_reassign_variable(components, body);
          break;
      }
      case AST_CALL_FXN: {
        _apl_gen_fxn_call_from_ast(components, body);
        break;
      }

      default: 
        break;

    }
  }
  _apl_emit_arena_set_mark(components, components->current_arena_ptr, mark_val);
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(components->builder))) {
    LLVMBuildBr(components->builder, stepBB);
  }

  LLVMPositionBuilderAtEnd(components->builder, stepBB);
  if (step != NULL) {
    if (step->Type == AST_VAR_ASS) {
      _apl_reassign_variable(components, step);
    } else if (step->Type == AST_VAR_DECL) {
      _apl_create_local_variable(components, step);
    } else if (step->Type == AST_CALL_FXN) {
      _apl_gen_fxn_call_from_ast(components, step);
    }
  }
  if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(components->builder))) {
    LLVMBuildBr(components->builder, condBB);
  }

  LLVMPositionBuilderAtEnd(components->builder, afterBB);
}
