#include "../headers/semantic.h"
#include "../headers/variables.h"
#include "../headers/functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Forward declarations for internal tree-walking functions
static void analyze_node(SemanticContext* context, ASTNode* node);
static void analyze_block(SemanticContext* context, ASTNode* block_node);

// Helper to extract line and column from any AST node
static void get_node_location(ASTNode *node, int *out_line, int *out_col) {
    if (!node) { *out_line = 0; *out_col = 0; return; }
    switch (node->Type) {
        case AST_PROGRAM:     *out_line = node->program.line;     *out_col = node->program.column; break;
        case AST_FUNCTION:    *out_line = node->function.line;    *out_col = node->function.column; break;
        case AST_BLOCK:       *out_line = node->block.line;       *out_col = node->block.column; break;
        case AST_VAR_DECL:    *out_line = node->var_decl.line;    *out_col = node->var_decl.column; break;
        case AST_PRINTLN:     *out_line = node->println.line;     *out_col = node->println.column; break;
        case AST_LITERAL_EXPR: *out_line = node->literal_expr.line; *out_col = node->literal_expr.column; break;
        case AST_BINARY_EXPR: *out_line = node->binary_expr.line;  *out_col = node->binary_expr.column; break;
        case AST_VAR_REF:     *out_line = node->var_ref.line;     *out_col = node->var_ref.column; break;
        case AST_VAR_ASS:     *out_line = node->var_assign.line;  *out_col = node->var_assign.column; break;
        case AST_CONCAT_STR:  *out_line = node->str_concat.line;  *out_col = node->str_concat.column; break;
        case AST_IF:          *out_line = node->if_stmt.line;     *out_col = node->if_stmt.column; break;
        case AST_WHILE:       *out_line = node->while_lp.line;    *out_col = node->while_lp.column; break;
        case AST_FOR:         *out_line = node->for_lp.line;      *out_col = node->for_lp.column; break;
        case AST_CALL_FXN:    *out_line = node->call_fxn.line;    *out_col = node->call_fxn.column; break;
        case AST_RET_NODE:    *out_line = node->ret_node.line;    *out_col = node->ret_node.column; break;
        default:             *out_line = 0;                      *out_col = 0; break;
    }
}

// Helper to push semantic errors
static void report_semantic_error(SemanticContext* context, ASTNode* node, const char* msg) {
    Error *e = calloc(1, sizeof(Error));
    e->type = SEMANTICERROR;
    e->message = strdup(msg);
    e->got = strdup("semantic_analysis");
    
    int line = 0, column = 0;
    get_node_location(node, &line, &column);
    
    e->line = line;
    e->column = column;
    errorStack_push(context->errors, e);
}

// Helper to infer expression type
static DataType infer_expr_type(SemanticContext *context, ASTNode *expr) {
    if (!expr) return TYPE_NULL;


    if (expr->Type == AST_LITERAL_EXPR) {
        if (expr->literal_expr.token.type == TOKEN_FLOAT) return TYPE_FLOAT;
        if (expr->literal_expr.token.type == TOKEN_BOOL) return TYPE_BOOL;
        if (expr->literal_expr.token.type == TOKEN_NULL) return TYPE_NULL;
        if (expr->literal_expr.token.type == TOKEN_STRING) return TYPE_STRING;
        return TYPE_INT;
    }

    if (expr->Type == AST_BINARY_EXPR) {
        DataType left_type = infer_expr_type(context, expr->binary_expr.left);
        DataType right_type = infer_expr_type(context, expr->binary_expr.right);
        return (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) ? TYPE_FLOAT : TYPE_INT;
    }

    if (expr->Type == AST_CONCAT_STR) {
        return TYPE_STRING;
    }

    if (expr->Type == AST_VAR_REF) {
        const int NAME_LENGTH = expr->var_ref.name_length;
        char *var_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
        memcpy(var_name, expr->var_ref.name, NAME_LENGTH);
        var_name[NAME_LENGTH] = '\0';
        Symbol *sym = lookup_token(context->codegen, var_name, &expr->var_ref.fxn, expr->var_ref.level);
        if (sym) {
            return sym->type;
        } else {
            report_semantic_error(context, expr, "Unrecognized variable referenced in expression.");
        }
    }

    if (expr->Type == AST_CALL_FXN) {
        const int NAME_LENGTH = expr->call_fxn.name_length;
        char *fn_name = alloc_space(NAME_LENGTH + 1, sizeof(char));
        memcpy(fn_name, expr->call_fxn.name, NAME_LENGTH);
        fn_name[NAME_LENGTH] = '\0';
        Symbol *sym = lookup_token(context->codegen, fn_name, &expr->call_fxn.fxn , expr->call_fxn.level);
        if (sym) {
            return sym->type;
        }
    }
    report_semantic_error(context, expr, "Could not infer expression type.");
    return TYPE_NULL;
}

// Main entry point
void analyze_semantics(SemanticContext* context, ASTNode* node) {
    if (!node) return;
    
    // Typically the root is an AST_PROGRAM
    analyze_node(context, node);
}

// Recursive tree-walker
static void analyze_node(SemanticContext* context, ASTNode* node) {
    if (!node) return;
    switch (node->Type) {
        case AST_PROGRAM:{
            if (node->program.function) {
                analyze_node(context, node->program.function);
            }
            break;
        }

        case AST_FUNCTION:{
            // Register function
            if (lookup_token(context->codegen, node->function.name, &node->function.fxn, node->function.level)) {
                report_semantic_error(context, node, "Cannot redefine function.");
            } else {
                node->function.resolved_symbol = register_fxn(context->codegen, node->function.name, node->function.fxn.return_type, node->function.level, &node->function.fxn);
            }

            //Analyze Parameters.
            Params *param = node->function.fxn.params;
            while(param){
                analyze_node(context, param->param);
                param = param->next;
            }

            //Analyze Function Body.
            if (node->function.body) {
                analyze_node(context, node->function.body);
            }


            break;
        }

        case AST_BLOCK:
            analyze_block(context, node);
            break;

        case AST_VAR_DECL: {
            
            const int NAME_LENGTH = node->var_decl.name_length;
            char name[NAME_LENGTH + 1];
            //snprintf(name, NAME_LENGTH+1, "%.*s", NAME_LENGTH, node->var_decl.name);
            memcpy(name, node->var_decl.name, NAME_LENGTH);
            name[NAME_LENGTH] = '\0';
            Symbol *sym = lookup_token(context->codegen, name, &node->var_decl.fxn, node->var_decl.level);
            if (sym) {
                report_semantic_error(context, node, "Multiple definition of variable.");
            }
            
            DataType inferred;
            if (node->var_decl.value) {
                analyze_node(context, node->var_decl.value);
               if(node->var_decl.value_type == TYPE_NULL){
               inferred = infer_expr_type(context, node->var_decl.value);
               node->var_decl.value_type = inferred;

               }else{
                DataType expected_type = node->var_decl.value_type;
                inferred = infer_expr_type(context, node->var_decl.value);
                if(inferred != expected_type){
                    report_semantic_error(context, node, "Datatype Mismatch.");
                }


               }

                register_variable(context->codegen, name, inferred, &node->var_decl.fxn,node->var_decl.level);
            

            } else {
                node->var_decl.value_type = TYPE_NULL;
                register_variable(context->codegen, name, TYPE_NULL,&node->var_decl.fxn,node->var_decl.level);
            }
            break;
        }

        case AST_VAR_ASS: {
            const int NAME_LENGTH = node->var_assign.name_length;
            char name[NAME_LENGTH + 1];
            //snprintf(name, NAME_LENGTH + 1, "%.*s", NAME_LENGTH, node->var_assign.name);

            memcpy(name, node->var_assign.name, NAME_LENGTH);
            name[NAME_LENGTH] = '\0';

            Symbol *sym = lookup_token(context->codegen, name, &node->var_assign.fxn, node->var_assign.level);
            if (!sym) {
                report_semantic_error(context, node, "Assignment to undeclared variable.");
            } else {
                node->var_assign.resolved_symbol = sym;
                if (node->var_assign.value) {
                    analyze_node(context, node->var_assign.value);
                    DataType inferred = infer_expr_type(context, node->var_assign.value);
                    
                    if (sym->type != TYPE_NULL && inferred != TYPE_NULL && sym->type != inferred) {
                        report_semantic_error(context, node, "Incompatible assignment type.");
                    } else if (sym->type == TYPE_NULL && inferred != TYPE_NULL) {
                        sym->type = inferred; // update symbol type if it was null
                    }
                }
            }
            break;
        }

        case AST_PRINTLN:
            if (node->println.value) {
                analyze_node(context, node->println.value);
            }
            break;

        case AST_IF:
            analyze_node(context, node->if_stmt.condition);
            analyze_node(context, node->if_stmt.then_block);
            if (node->if_stmt.else_block) {
                analyze_node(context, node->if_stmt.else_block);
            }
            break;

        case AST_WHILE:
            analyze_node(context, node->while_lp.condition);
            analyze_node(context, node->while_lp.then_block);
            break;

        case AST_FOR:
            analyze_node(context, node->for_lp.variable);
            analyze_node(context, node->for_lp.condtion);
            analyze_node(context, node->for_lp.var_operation);
            analyze_node(context, node->for_lp.then_block);
            break;

        case AST_BINARY_EXPR:
            analyze_node(context, node->binary_expr.left);
            analyze_node(context, node->binary_expr.right);
            break;

        case AST_CONCAT_STR:
            analyze_node(context, node->str_concat.left);
            analyze_node(context, node->str_concat.right);
            break;

        case AST_RET_NODE:
            if (node->ret_node.value) {
                analyze_node(context, node->ret_node.value);
            }
            break;
            
        case AST_CALL_FXN: {
            
            if(memcmp(node->call_fxn.name, "run", 3) == 0){
                report_semantic_error(context, node, "Cannot call entry-point function");
                
            }

            Symbol *sym = lookup_token(context->codegen, node->call_fxn.name, &node->call_fxn.fxn, node->call_fxn.level);
            if (!sym) {
                printf("UNDECLARED FUNCTION\n");
                report_semantic_error(context, node, "Call to undeclared function.");
            } else {
                node->call_fxn.return_type = sym->type;
                node->call_fxn.resolved_symbol = sym;
                if(sym->fxn->params && node->call_fxn.args){
                                    node->call_fxn.return_type = sym->type;
                node->call_fxn.resolved_symbol = sym;
                
                Params *expected_param = sym->fxn->params;
                Args *passed_arg = node->call_fxn.args;
                
                while (expected_param != NULL && passed_arg != NULL) {
                    analyze_node(context, passed_arg->arg);
                    
                    DataType arg_type = infer_expr_type(context, passed_arg->arg);
                    if (arg_type != expected_param->param->var_decl.value_type) {
                        report_semantic_error(context, passed_arg->arg, "Argument type mismatch in function call.");
                    }
                    
                    expected_param = expected_param->next;
                    passed_arg = passed_arg->next;
                }
                    
                }

            }

            break;
        }

        case AST_LITERAL_EXPR:
            break;

        case AST_VAR_REF:{
            const int NAME_LENGTH = node->var_ref.name_length;
            char name[NAME_LENGTH + 1];
            //snprintf(name, NAME_LENGTH + 1, "%.*s", NAME_LENGTH, node->var_ref.name);
            memcpy(name,node->var_ref.name, NAME_LENGTH);
            name[NAME_LENGTH] = '\0';

            Symbol *sym = lookup_token(context->codegen, name, &node->var_ref.fxn, node->var_ref.level);
            if (!sym) {
                report_semantic_error(context, node, "Referenced Variable not found in the current scope.");
            } else {
                node->var_ref.resolved_symbol = sym;
            }
            break;
        }
        default:
            break;
    }
}

// Walks through all statements in a block
static void analyze_block(SemanticContext* context, ASTNode* block_node) {
    if (!block_node || block_node->Type != AST_BLOCK) return;

    int initial_symbol_count = context->codegen->symbol_count;

    for (int i = 0; i < block_node->block.count; ++i) {
        analyze_node(context, block_node->block.statements[i]);
    }

    for (int i = initial_symbol_count; i < context->codegen->symbol_count; ++i) {
        context->codegen->symbols[i].is_active = false;
    }
}


