#include "ast.h"
#include <stdlib.h>
#include <string.h>

void node_list_init(NodeList *list) {
    list->items = NULL;
    list->count = 0;
    list->cap = 0;
}

void node_list_push(NodeList *list, AstNode *node) {
    if (list->count >= list->cap) {
        list->cap = list->cap ? list->cap * 2 : 8;
        list->items = (AstNode **)realloc(list->items, sizeof(AstNode *) * list->cap);
    }
    list->items[list->count++] = node;
}

void param_list_init(ParamList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void param_list_push(ParamList *list, Param p) {
    if (list->count + 1 > list->capacity) {
        int new_cap = list->capacity < 8 ? 8 : list->capacity * 2;
        Param *new_items = (Param *)realloc(list->items, new_cap * sizeof(Param));
        if (!new_items) {
            fprintf(stderr, "Fatal error: Out of memory in param_list_push\n");
            exit(1);
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = p;
}

AstNode *ast_new(NodeType type, int line) {
    AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
    n->type = type;
    n->line = line;
    return n;
}


void ast_free(AstNode *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->program.stmts.count; i++)
                ast_free(node->program.stmts.items[i]);
            free(node->program.stmts.items);
            break;
        case NODE_FUNC_DEF:
            free(node->func_def.name);
            free(node->func_def.ret_type);
            for (int i = 0; i < node->func_def.params.count; i++) {
                free(node->func_def.params.items[i].name);
                free(node->func_def.params.items[i].type_ann);
            }
            free(node->func_def.params.items);
            for (int i = 0; i < node->func_def.body.count; i++)
                ast_free(node->func_def.body.items[i]);
            free(node->func_def.body.items);
            break;
        case NODE_CLASS_DEF:
            free(node->class_def.name);
            for (int i = 0; i < node->class_def.body.count; i++)
                ast_free(node->class_def.body.items[i]);
            free(node->class_def.body.items);
            break;
        case NODE_IF:
            ast_free(node->if_stmt.cond);
            for (int i = 0; i < node->if_stmt.then_body.count; i++)
                ast_free(node->if_stmt.then_body.items[i]);
            free(node->if_stmt.then_body.items);
            ast_free(node->if_stmt.else_branch);
            break;
        case NODE_FOR:
            free(node->for_stmt.var);
            ast_free(node->for_stmt.iter);
            for (int i = 0; i < node->for_stmt.body.count; i++)
                ast_free(node->for_stmt.body.items[i]);
            free(node->for_stmt.body.items);
            break;
        case NODE_WHILE:
            ast_free(node->while_stmt.cond);
            for (int i = 0; i < node->while_stmt.body.count; i++)
                ast_free(node->while_stmt.body.items[i]);
            free(node->while_stmt.body.items);
            break;
        case NODE_RETURN:
            ast_free(node->return_stmt.value);
            break;
        case NODE_ASSIGN:
            free(node->assign.name);
            free(node->assign.type_ann);
            ast_free(node->assign.value);
            break;
        case NODE_AUG_ASSIGN:
            free(node->aug_assign.name);
            ast_free(node->aug_assign.value);
            break;
        case NODE_SUBSCRIPT_ASSIGN:
            ast_free(node->subscript_assign.obj);
            ast_free(node->subscript_assign.index);
            ast_free(node->subscript_assign.value);
            break;
        case NODE_EXPR_STMT:
            ast_free(node->expr_stmt.expr);
            break;
        case NODE_CONST_DECL:
            free(node->const_decl.name);
            ast_free(node->const_decl.value);
            break;
        case NODE_BIN_OP:
            ast_free(node->bin_op.left);
            ast_free(node->bin_op.right);
            break;
        case NODE_UNARY_OP:
            ast_free(node->unary_op.operand);
            break;
        case NODE_CALL:
            ast_free(node->call.func);
            for (int i = 0; i < node->call.args.count; i++)
                ast_free(node->call.args.items[i]);
            free(node->call.args.items);
            if (node->call.is_unpacked_list) free(node->call.is_unpacked_list);
            if (node->call.is_unpacked_dict) free(node->call.is_unpacked_dict);
            break;
        case NODE_NAME:
            free(node->name_expr.name);
            break;
        case NODE_STRING_LIT:
            free(node->str_lit.value);
            break;
        case NODE_LIST_EXPR:
            for (int i = 0; i < node->list_expr.elems.count; i++)
                ast_free(node->list_expr.elems.items[i]);
            free(node->list_expr.elems.items);
            break;
        case NODE_SUBSCRIPT:
            ast_free(node->subscript.obj);
            ast_free(node->subscript.index);
            break;
        case NODE_DICT_EXPR:
            for (int i = 0; i < node->dict_expr.keys.count; i++) {
                ast_free(node->dict_expr.keys.items[i]);
                ast_free(node->dict_expr.values.items[i]);
            }
            free(node->dict_expr.keys.items);
            free(node->dict_expr.values.items);
            break;
        case NODE_SET_EXPR:
            for (int i = 0; i < node->set_expr.elems.count; i++)
                ast_free(node->set_expr.elems.items[i]);
            free(node->set_expr.elems.items);
            break;
        case NODE_TUPLE_EXPR:
            for (int i = 0; i < node->tuple_expr.elems.count; i++)
                ast_free(node->tuple_expr.elems.items[i]);
            free(node->tuple_expr.elems.items);
            break;
        case NODE_ATTRIBUTE:
            ast_free(node->attribute.obj);
            free(node->attribute.attr);
            break;
        case NODE_IMPORT:
            free(node->import_stmt.module);
            free(node->import_stmt.alias);
            break;
        case NODE_WITH:
            ast_free(node->with_stmt.expr);
            free(node->with_stmt.alias);
            for (int i = 0; i < node->with_stmt.body.count; i++)
                ast_free(node->with_stmt.body.items[i]);
            free(node->with_stmt.body.items);
            break;
        case NODE_TRY:
            for (int i = 0; i < node->try_stmt.body.count; i++)
                ast_free(node->try_stmt.body.items[i]);
            free(node->try_stmt.body.items);
            for (int i = 0; i < node->try_stmt.except_body.count; i++)
                ast_free(node->try_stmt.except_body.items[i]);
            free(node->try_stmt.except_body.items);
            free(node->try_stmt.except_type);
            free(node->try_stmt.except_alias);
            for (int i = 0; i < node->try_stmt.finally_body.count; i++)
                ast_free(node->try_stmt.finally_body.items[i]);
            free(node->try_stmt.finally_body.items);
            break;
        case NODE_RAISE:
            if (node->raise_stmt.exc) ast_free(node->raise_stmt.exc);
            break;
        default:
            break;
    }
    free(node);
}
