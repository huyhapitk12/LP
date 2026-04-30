#include "type_inference.h"
#include <string.h>
#include <stdio.h>

void type_inference_init(TypeEnv *env) {
    env->count = 0;
    env->parent = NULL;
}

static void env_set(TypeEnv *env, const char *name, LpType type) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            /* Last-Assignment-Wins strategy: update the type directly without fallback */
            env->vars[i].type = type;
            return;
        }
    }
    if (env->count < 512) {
        env->vars[env->count].name = (char*)name;
        env->vars[env->count].type = type;
        env->count++;
    }
}

static LpType env_get(TypeEnv *env, const char *name) {
    TypeEnv *curr = env;
    while (curr) {
        for (int i = 0; i < curr->count; i++) {
            if (strcmp(curr->vars[i].name, name) == 0) {
                return curr->vars[i].type;
            }
        }
        curr = curr->parent;
    }
    return LP_UNKNOWN; /* -1 effectively or unknown */
}

void type_infer_node(TypeEnv *env, AstNode *node) {
    if (!node) return;
    
    /* Pre-set to unknown by default, unless proven otherwise */
    node->inferred_type = LP_UNKNOWN;
    
    switch (node->type) {
        case NODE_INT_LIT:
            node->inferred_type = LP_INT;
            break;
        case NODE_FLOAT_LIT:
            node->inferred_type = LP_FLOAT;
            break;
        case NODE_STRING_LIT:
            node->inferred_type = LP_STRING;
            break;
        case NODE_BOOL_LIT:
            node->inferred_type = LP_BOOL;
            break;
        case NODE_ASSIGN:
            type_infer_node(env, node->assign.value);
            if (node->assign.value) {
                env_set(env, node->assign.name, node->assign.value->inferred_type);
                node->inferred_type = node->assign.value->inferred_type;
            }
            break;
        case NODE_NAME: {
            LpType t = env_get(env, node->name_expr.name);
            if (t != LP_UNKNOWN) node->inferred_type = t;
            else node->inferred_type = LP_VAL; /* fallback */
            break;
        }
        case NODE_BIN_OP:
            type_infer_node(env, node->bin_op.left);
            type_infer_node(env, node->bin_op.right);
            if (node->bin_op.left && node->bin_op.right) {
                if (node->bin_op.left->inferred_type == LP_FLOAT || node->bin_op.right->inferred_type == LP_FLOAT)
                    node->inferred_type = LP_FLOAT;
                else if (node->bin_op.left->inferred_type == LP_INT && node->bin_op.right->inferred_type == LP_INT)
                    node->inferred_type = LP_INT;
                else if (node->bin_op.left->inferred_type == LP_STRING || node->bin_op.right->inferred_type == LP_STRING)
                    node->inferred_type = LP_STRING;
                else
                    node->inferred_type = LP_VAL; /* Fallback for mixed/unknown */
            }
            break;
        case NODE_PROGRAM:
            for (int i = 0; i < node->program.stmts.count; i++) {
                type_infer_node(env, node->program.stmts.items[i]);
            }
            break;
        case NODE_FUNC_DEF: {
            TypeEnv child_env;
            type_inference_init(&child_env);
            child_env.parent = env;
            /* Monomorphization mapping handles parameter types. For generic analysis we default params to LP_VAL */
            for (int i = 0; i < node->func_def.params.count; i++) {
                env_set(&child_env, node->func_def.params.items[i].name, LP_VAL);
            }
            for (int i = 0; i < node->func_def.body.count; i++) {
                type_infer_node(&child_env, node->func_def.body.items[i]);
            }
            break;
        }
        case NODE_IF:
            type_infer_node(env, node->if_stmt.cond);
            for (int i = 0; i < node->if_stmt.then_body.count; i++) {
                type_infer_node(env, node->if_stmt.then_body.items[i]);
            }
            if (node->if_stmt.else_branch) {
                type_infer_node(env, node->if_stmt.else_branch);
            }
            break;
        case NODE_WHILE:
            type_infer_node(env, node->while_stmt.cond);
            for (int i = 0; i < node->while_stmt.body.count; i++) {
                type_infer_node(env, node->while_stmt.body.items[i]);
            }
            break;
        case NODE_RETURN:
            type_infer_node(env, node->return_stmt.value);
            if (node->return_stmt.value) {
                node->inferred_type = node->return_stmt.value->inferred_type;
            }
            break;
        case NODE_EXPR_STMT:
            type_infer_node(env, node->expr_stmt.expr);
            break;
        case NODE_LIST_EXPR:
            for (int i = 0; i < node->list_expr.elems.count; i++) {
                type_infer_node(env, node->list_expr.elems.items[i]);
            }
            /* Could check if all elements are int/float to infer native array, but keeping LP_LIST for now */
            node->inferred_type = LP_LIST;
            break;
        case NODE_CALL:
            type_infer_node(env, node->call.func);
            for (int i = 0; i < node->call.args.count; i++) {
                type_infer_node(env, node->call.args.items[i]);
            }
            /* Assume basic fallback since functions are dynamically typed */
            node->inferred_type = LP_VAL;
            break;
        case NODE_FOR:
            type_infer_node(env, node->for_stmt.iter);
            if (node->for_stmt.iter && node->for_stmt.iter->type == NODE_CALL &&
                node->for_stmt.iter->call.func->type == NODE_NAME &&
                strcmp(node->for_stmt.iter->call.func->name_expr.name, "range") == 0) {
                env_set(env, node->for_stmt.var, LP_INT); /* Iterating range() sets var to int */
            } else {
                env_set(env, node->for_stmt.var, LP_VAL); /* Fallback */
            }
            for (int i = 0; i < node->for_stmt.body.count; i++) {
                type_infer_node(env, node->for_stmt.body.items[i]);
            }
            break;
        default:
            /* Ignore type inference for anything complex right now to stick to LP_VAL fallback */
            break;
    }
}

void type_infer_program(TypeEnv *env, AstNode *program) {
    if (program && program->type == NODE_PROGRAM) {
        /* Fixed-point iteration (Max 5 passes) */
        for (int pass = 0; pass < 5; pass++) {
            type_infer_node(env, program);
        }
    }
}
