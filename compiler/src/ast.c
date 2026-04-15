#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

void type_param_list_init(TypeParamList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void type_param_list_push(TypeParamList *list, TypeParam p) {
    if (list->count + 1 > list->capacity) {
        int new_cap = list->capacity < 8 ? 8 : list->capacity * 2;
        TypeParam *new_items = (TypeParam *)realloc(list->items, new_cap * sizeof(TypeParam));
        if (!new_items) {
            fprintf(stderr, "Fatal error: Out of memory in type_param_list_push\n");
            exit(1);
        }
        list->items = new_items;
        list->capacity = new_cap;
    }
    list->items[list->count++] = p;
}

AstNode *ast_new(LpArena *arena, NodeType type, int line) {
    AstNode *n = (AstNode *)lp_memory_arena_alloc(arena, sizeof(AstNode));
    if (n) {
        memset(n, 0, sizeof(AstNode));
        n->type = type;
        n->line = line;
    }
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
            for (int i = 0; i < node->func_def.type_params.count; i++) {
                free(node->func_def.type_params.items[i].name);
            }
            free(node->func_def.type_params.items);
            for (int i = 0; i < node->func_def.params.count; i++) {
                free(node->func_def.params.items[i].name);
                free(node->func_def.params.items[i].type_ann);
            }
            free(node->func_def.params.items);
            for (int i = 0; i < node->func_def.body.count; i++)
                ast_free(node->func_def.body.items[i]);
            free(node->func_def.body.items);
            for (int i = 0; i < node->func_def.decorators.count; i++)
                ast_free(node->func_def.decorators.items[i]);
            free(node->func_def.decorators.items);
            break;
        case NODE_CLASS_DEF:
            free(node->class_def.name);
            free(node->class_def.base_class);
            for (int i = 0; i < node->class_def.type_params.count; i++) {
                free(node->class_def.type_params.items[i].name);
            }
            free(node->class_def.type_params.items);
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
        case NODE_FSTRING:
            for (int i = 0; i < node->fstring_expr.parts.count; i++)
                ast_free(node->fstring_expr.parts.items[i]);
            free(node->fstring_expr.parts.items);
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
        case NODE_MATCH:
            ast_free(node->match_stmt.value);
            for (int i = 0; i < node->match_stmt.cases.count; i++)
                ast_free(node->match_stmt.cases.items[i]);
            free(node->match_stmt.cases.items);
            break;
        case NODE_MATCH_CASE:
            ast_free(node->match_case.pattern);
            ast_free(node->match_case.guard);
            for (int i = 0; i < node->match_case.body.count; i++)
                ast_free(node->match_case.body.items[i]);
            free(node->match_case.body.items);
            break;
        case NODE_GENERIC_INST:
            free(node->generic_inst.base_name);
            for (int i = 0; i < node->generic_inst.type_args.count; i++)
                ast_free(node->generic_inst.type_args.items[i]);
            free(node->generic_inst.type_args.items);
            break;
        case NODE_TYPE_UNION:
            if (node->type_union.types) {
                for (int i = 0; i < node->type_union.count; i++) {
                    free(node->type_union.types[i]);
                }
                free(node->type_union.types);
            }
            break;
        case NODE_ASYNC_DEF:
            free(node->async_def.name);
            free(node->async_def.ret_type);
            for (int i = 0; i < node->async_def.params.count; i++) {
                free(node->async_def.params.items[i].name);
                free(node->async_def.params.items[i].type_ann);
            }
            free(node->async_def.params.items);
            for (int i = 0; i < node->async_def.body.count; i++)
                ast_free(node->async_def.body.items[i]);
            free(node->async_def.body.items);
            for (int i = 0; i < node->async_def.decorators.count; i++)
                ast_free(node->async_def.decorators.items[i]);
            free(node->async_def.decorators.items);
            break;
        case NODE_AWAIT_EXPR:
            ast_free(node->await_expr.expr);
            break;
        case NODE_SECURITY:
            free(node->security.auth_type);
            free(node->security.cors_origins);
            free(node->security.hash_algorithm);
            break;
        default:
            break;
    }
}

/* --- AST Debug Dump --- */

static const char *node_type_name(NodeType t) {
    switch (t) {
        case NODE_PROGRAM:       return "PROGRAM";
        case NODE_FUNC_DEF:      return "FUNC_DEF";
        case NODE_CLASS_DEF:     return "CLASS_DEF";
        case NODE_IF:            return "IF";
        case NODE_FOR:           return "FOR";
        case NODE_WHILE:         return "WHILE";
        case NODE_RETURN:        return "RETURN";
        case NODE_ASSIGN:        return "ASSIGN";
        case NODE_AUG_ASSIGN:    return "AUG_ASSIGN";
        case NODE_SUBSCRIPT_ASSIGN: return "SUBSCRIPT_ASSIGN";
        case NODE_EXPR_STMT:     return "EXPR_STMT";
        case NODE_PASS:          return "PASS";
        case NODE_BREAK:         return "BREAK";
        case NODE_CONTINUE:      return "CONTINUE";
        case NODE_CONST_DECL:    return "CONST_DECL";
        case NODE_IMPORT:        return "IMPORT";
        case NODE_WITH:          return "WITH";
        case NODE_BIN_OP:        return "BIN_OP";
        case NODE_UNARY_OP:      return "UNARY_OP";
        case NODE_CALL:          return "CALL";
        case NODE_NAME:          return "NAME";
        case NODE_KWARG:         return "KWARG";
        case NODE_INT_LIT:       return "INT_LIT";
        case NODE_FLOAT_LIT:     return "FLOAT_LIT";
        case NODE_STRING_LIT:    return "STRING_LIT";
        case NODE_FSTRING:       return "FSTRING";
        case NODE_BOOL_LIT:      return "BOOL_LIT";
        case NODE_NONE_LIT:      return "NONE_LIT";
        case NODE_LIST_EXPR:     return "LIST_EXPR";
        case NODE_DICT_EXPR:     return "DICT_EXPR";
        case NODE_SET_EXPR:      return "SET_EXPR";
        case NODE_TUPLE_EXPR:    return "TUPLE_EXPR";
        case NODE_SUBSCRIPT:     return "SUBSCRIPT";
        case NODE_ATTRIBUTE:     return "ATTRIBUTE";
        case NODE_TRY:           return "TRY";
        case NODE_RAISE:         return "RAISE";
        case NODE_LIST_COMP:     return "LIST_COMP";
        case NODE_DICT_COMP:     return "DICT_COMP";
        case NODE_LAMBDA:        return "LAMBDA";
        case NODE_YIELD:         return "YIELD";
        case NODE_PARALLEL_FOR:  return "PARALLEL_FOR";
        case NODE_SETTINGS:      return "SETTINGS";
        case NODE_PARALLEL_SETTINGS: return "PARALLEL_SETTINGS";
        case NODE_SECURITY:      return "SECURITY";
        case NODE_MATCH:         return "MATCH";
        case NODE_MATCH_CASE:    return "MATCH_CASE";
        case NODE_ASYNC_DEF:     return "ASYNC_DEF";
        case NODE_AWAIT_EXPR:    return "AWAIT_EXPR";
        case NODE_TYPE_UNION:    return "TYPE_UNION";
        case NODE_GENERIC_INST:  return "GENERIC_INST";
        default:                 return "UNKNOWN";
    }
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

void ast_dump(AstNode *node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }

    print_indent(indent);
    printf("%s [line %d]", node_type_name(node->type), node->line);

    switch (node->type) {
        case NODE_PROGRAM:
            printf(" (%d stmts)\n", node->program.stmts.count);
            for (int i = 0; i < node->program.stmts.count; i++)
                ast_dump(node->program.stmts.items[i], indent + 1);
            break;
        case NODE_FUNC_DEF:
            printf(" name=%s params=%d ret=%s\n",
                   node->func_def.name ? node->func_def.name : "(null)",
                   node->func_def.params.count,
                   node->func_def.ret_type ? node->func_def.ret_type : "void");
            for (int i = 0; i < node->func_def.body.count; i++)
                ast_dump(node->func_def.body.items[i], indent + 1);
            break;
        case NODE_CLASS_DEF:
            printf(" name=%s base=%s\n",
                   node->class_def.name ? node->class_def.name : "(null)",
                   node->class_def.base_class ? node->class_def.base_class : "(none)");
            for (int i = 0; i < node->class_def.body.count; i++)
                ast_dump(node->class_def.body.items[i], indent + 1);
            break;
        case NODE_IF:
            printf("\n");
            print_indent(indent + 1); printf("cond:\n");
            ast_dump(node->if_stmt.cond, indent + 2);
            print_indent(indent + 1); printf("then: (%d stmts)\n", node->if_stmt.then_body.count);
            for (int i = 0; i < node->if_stmt.then_body.count; i++)
                ast_dump(node->if_stmt.then_body.items[i], indent + 2);
            if (node->if_stmt.else_branch) {
                print_indent(indent + 1); printf("else:\n");
                ast_dump(node->if_stmt.else_branch, indent + 2);
            }
            break;
        case NODE_FOR:
            printf(" var=%s\n", node->for_stmt.var ? node->for_stmt.var : "(null)");
            print_indent(indent + 1); printf("iter:\n");
            ast_dump(node->for_stmt.iter, indent + 2);
            for (int i = 0; i < node->for_stmt.body.count; i++)
                ast_dump(node->for_stmt.body.items[i], indent + 1);
            break;
        case NODE_WHILE:
            printf("\n");
            ast_dump(node->while_stmt.cond, indent + 1);
            for (int i = 0; i < node->while_stmt.body.count; i++)
                ast_dump(node->while_stmt.body.items[i], indent + 1);
            break;
        case NODE_RETURN:
            printf("\n");
            if (node->return_stmt.value)
                ast_dump(node->return_stmt.value, indent + 1);
            break;
        case NODE_ASSIGN:
            printf(" name=%s type=%s\n",
                   node->assign.name ? node->assign.name : "(null)",
                   node->assign.type_ann ? node->assign.type_ann : "(inferred)");
            if (node->assign.value)
                ast_dump(node->assign.value, indent + 1);
            break;
        case NODE_AUG_ASSIGN:
            printf(" name=%s op=%s\n",
                   node->aug_assign.name ? node->aug_assign.name : "(null)",
                   token_type_name(node->aug_assign.op));
            if (node->aug_assign.value)
                ast_dump(node->aug_assign.value, indent + 1);
            break;
        case NODE_EXPR_STMT:
            printf("\n");
            ast_dump(node->expr_stmt.expr, indent + 1);
            break;
        case NODE_BIN_OP:
            printf(" op=%s\n", token_type_name(node->bin_op.op));
            ast_dump(node->bin_op.left, indent + 1);
            ast_dump(node->bin_op.right, indent + 1);
            break;
        case NODE_UNARY_OP:
            printf(" op=%s\n", token_type_name(node->unary_op.op));
            ast_dump(node->unary_op.operand, indent + 1);
            break;
        case NODE_CALL:
            printf(" (%d args)\n", node->call.args.count);
            print_indent(indent + 1); printf("func:\n");
            ast_dump(node->call.func, indent + 2);
            for (int i = 0; i < node->call.args.count; i++)
                ast_dump(node->call.args.items[i], indent + 1);
            break;
        case NODE_NAME:
            printf(" \"%s\"\n", node->name_expr.name ? node->name_expr.name : "(null)");
            break;
        case NODE_INT_LIT:
            printf(" %" PRId64 "\n", node->int_lit.value);
            break;
        case NODE_FLOAT_LIT:
            printf(" %g\n", node->float_lit.value);
            break;
        case NODE_STRING_LIT:
            printf(" \"%s\"\n", node->str_lit.value ? node->str_lit.value : "");
            break;
        case NODE_BOOL_LIT:
            printf(" %s\n", node->bool_lit.value ? "True" : "False");
            break;
        case NODE_NONE_LIT:
            printf("\n");
            break;
        case NODE_IMPORT:
            printf(" module=%s alias=%s\n",
                   node->import_stmt.module ? node->import_stmt.module : "(null)",
                   node->import_stmt.alias ? node->import_stmt.alias : "(none)");
            break;
        case NODE_ATTRIBUTE:
            printf(" .%s\n", node->attribute.attr ? node->attribute.attr : "(null)");
            ast_dump(node->attribute.obj, indent + 1);
            break;
        case NODE_SUBSCRIPT:
            printf("\n");
            ast_dump(node->subscript.obj, indent + 1);
            ast_dump(node->subscript.index, indent + 1);
            break;
        case NODE_LIST_EXPR:
            printf(" (%d elems)\n", node->list_expr.elems.count);
            for (int i = 0; i < node->list_expr.elems.count; i++)
                ast_dump(node->list_expr.elems.items[i], indent + 1);
            break;
        case NODE_DICT_EXPR:
            printf(" (%d pairs)\n", node->dict_expr.keys.count);
            for (int i = 0; i < node->dict_expr.keys.count; i++) {
                print_indent(indent + 1); printf("key:\n");
                ast_dump(node->dict_expr.keys.items[i], indent + 2);
                print_indent(indent + 1); printf("val:\n");
                ast_dump(node->dict_expr.values.items[i], indent + 2);
            }
            break;
        case NODE_MATCH:
            printf("\n");
            print_indent(indent + 1); printf("value:\n");
            ast_dump(node->match_stmt.value, indent + 2);
            for (int i = 0; i < node->match_stmt.cases.count; i++)
                ast_dump(node->match_stmt.cases.items[i], indent + 1);
            break;
        case NODE_MATCH_CASE:
            printf(" wildcard=%d\n", node->match_case.is_wildcard);
            if (node->match_case.pattern)
                ast_dump(node->match_case.pattern, indent + 1);
            if (node->match_case.guard) {
                print_indent(indent + 1); printf("guard:\n");
                ast_dump(node->match_case.guard, indent + 2);
            }
            for (int i = 0; i < node->match_case.body.count; i++)
                ast_dump(node->match_case.body.items[i], indent + 1);
            break;
        case NODE_KWARG:
            printf(" %s=\n", node->kwarg.name ? node->kwarg.name : "(null)");
            ast_dump(node->kwarg.value, indent + 1);
            break;
        case NODE_CONST_DECL:
            printf(" name=%s\n", node->const_decl.name ? node->const_decl.name : "(null)");
            ast_dump(node->const_decl.value, indent + 1);
            break;
        case NODE_TRY:
            printf("\n");
            print_indent(indent + 1); printf("body:\n");
            for (int i = 0; i < node->try_stmt.body.count; i++)
                ast_dump(node->try_stmt.body.items[i], indent + 2);
            if (node->try_stmt.except_body.count > 0) {
                print_indent(indent + 1); printf("except %s:\n",
                    node->try_stmt.except_type ? node->try_stmt.except_type : "");
                for (int i = 0; i < node->try_stmt.except_body.count; i++)
                    ast_dump(node->try_stmt.except_body.items[i], indent + 2);
            }
            break;
        case NODE_GENERIC_INST:
            printf(" base=%s (%d type_args)\n",
                   node->generic_inst.base_name ? node->generic_inst.base_name : "(null)",
                   node->generic_inst.type_args.count);
            break;
        case NODE_ASYNC_DEF:
            printf(" name=%s\n",
                   node->async_def.name ? node->async_def.name : "(null)");
            for (int i = 0; i < node->async_def.body.count; i++)
                ast_dump(node->async_def.body.items[i], indent + 1);
            break;
        case NODE_AWAIT_EXPR:
            printf("\n");
            ast_dump(node->await_expr.expr, indent + 1);
            break;
        case NODE_LAMBDA:
            printf(" params=%d multiline=%d\n",
                   node->lambda_expr.params.count,
                   node->lambda_expr.is_multiline);
            if (node->lambda_expr.body)
                ast_dump(node->lambda_expr.body, indent + 1);
            for (int i = 0; i < node->lambda_expr.body_stmts.count; i++)
                ast_dump(node->lambda_expr.body_stmts.items[i], indent + 1);
            break;
        case NODE_FSTRING:
            printf(" (%d parts)\n", node->fstring_expr.parts.count);
            for (int i = 0; i < node->fstring_expr.parts.count; i++)
                ast_dump(node->fstring_expr.parts.items[i], indent + 1);
            break;
        default:
            printf("\n");
            break;
    }
}
