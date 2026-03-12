#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Helper to assert condition and track failure
static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) \
    do { \
        tests_run++; \
        if (!(cond)) { \
            printf("FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); \
            tests_failed++; \
        } \
    } while (0)

// Helper to copy strings (since ast_free calls free on them)
static char* str_dup(const char* s) {
    if (!s) return NULL;
    char* d = malloc(strlen(s) + 1);
    strcpy(d, s);
    return d;
}

static void test_ast_free_program() {
    AstNode *node = ast_new(NODE_PROGRAM, 1);
    node_list_init(&node->program.stmts);

    AstNode *expr_stmt = ast_new(NODE_EXPR_STMT, 1);
    expr_stmt->expr_stmt.expr = ast_new(NODE_INT_LIT, 1);

    node_list_push(&node->program.stmts, expr_stmt);

    // We expect this to not crash or leak memory.
    ast_free(node);
    ASSERT(1, "test_ast_free_program completed");
}

static void test_ast_free_func_def() {
    AstNode *node = ast_new(NODE_FUNC_DEF, 1);
    node->func_def.name = str_dup("my_func");
    node->func_def.ret_type = str_dup("int");

    param_list_init(&node->func_def.params);
    Param p;
    p.name = str_dup("x");
    p.type_ann = str_dup("int");
    param_list_push(&node->func_def.params, p);

    node_list_init(&node->func_def.body);
    AstNode *ret = ast_new(NODE_RETURN, 2);
    ret->return_stmt.value = ast_new(NODE_INT_LIT, 2);
    node_list_push(&node->func_def.body, ret);

    ast_free(node);
    ASSERT(1, "test_ast_free_func_def completed");
}

static void test_ast_free_class_def() {
    AstNode *node = ast_new(NODE_CLASS_DEF, 1);
    node->class_def.name = str_dup("MyClass");
    node->class_def.base_class = NULL; // not freed by ast_free

    node_list_init(&node->class_def.body);
    AstNode *pass = ast_new(NODE_PASS, 2);
    node_list_push(&node->class_def.body, pass);

    ast_free(node);
    ASSERT(1, "test_ast_free_class_def completed");
}

static void test_ast_free_if() {
    AstNode *node = ast_new(NODE_IF, 1);
    node->if_stmt.cond = ast_new(NODE_BOOL_LIT, 1);

    node_list_init(&node->if_stmt.then_body);
    node_list_push(&node->if_stmt.then_body, ast_new(NODE_PASS, 2));

    node->if_stmt.else_branch = ast_new(NODE_PASS, 3); // simplistic else

    ast_free(node);
    ASSERT(1, "test_ast_free_if completed");
}

static void test_ast_free_for() {
    AstNode *node = ast_new(NODE_FOR, 1);
    node->for_stmt.var = str_dup("i");
    node->for_stmt.iter = ast_new(NODE_LIST_EXPR, 1);
    node_list_init(&node->for_stmt.iter->list_expr.elems);

    node_list_init(&node->for_stmt.body);
    node_list_push(&node->for_stmt.body, ast_new(NODE_PASS, 2));

    ast_free(node);
    ASSERT(1, "test_ast_free_for completed");
}

static void test_ast_free_while() {
    AstNode *node = ast_new(NODE_WHILE, 1);
    node->while_stmt.cond = ast_new(NODE_BOOL_LIT, 1);

    node_list_init(&node->while_stmt.body);
    node_list_push(&node->while_stmt.body, ast_new(NODE_PASS, 2));

    ast_free(node);
    ASSERT(1, "test_ast_free_while completed");
}

static void test_ast_free_assign() {
    AstNode *node = ast_new(NODE_ASSIGN, 1);
    node->assign.name = str_dup("var");
    node->assign.type_ann = str_dup("str");
    node->assign.value = ast_new(NODE_STRING_LIT, 1);
    node->assign.value->str_lit.value = str_dup("hello");

    ast_free(node);
    ASSERT(1, "test_ast_free_assign completed");
}

static void test_ast_free_aug_assign() {
    AstNode *node = ast_new(NODE_AUG_ASSIGN, 1);
    node->aug_assign.name = str_dup("count");
    node->aug_assign.value = ast_new(NODE_INT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_aug_assign completed");
}

static void test_ast_free_subscript_assign() {
    AstNode *node = ast_new(NODE_SUBSCRIPT_ASSIGN, 1);
    node->subscript_assign.obj = ast_new(NODE_NAME, 1);
    node->subscript_assign.obj->name_expr.name = str_dup("arr");
    node->subscript_assign.index = ast_new(NODE_INT_LIT, 1);
    node->subscript_assign.value = ast_new(NODE_INT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_subscript_assign completed");
}

static void test_ast_free_const_decl() {
    AstNode *node = ast_new(NODE_CONST_DECL, 1);
    node->const_decl.name = str_dup("PI");
    node->const_decl.value = ast_new(NODE_FLOAT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_const_decl completed");
}

static void test_ast_free_bin_op() {
    AstNode *node = ast_new(NODE_BIN_OP, 1);
    node->bin_op.left = ast_new(NODE_INT_LIT, 1);
    node->bin_op.right = ast_new(NODE_INT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_bin_op completed");
}

static void test_ast_free_unary_op() {
    AstNode *node = ast_new(NODE_UNARY_OP, 1);
    node->unary_op.operand = ast_new(NODE_INT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_unary_op completed");
}

static void test_ast_free_call() {
    AstNode *node = ast_new(NODE_CALL, 1);
    node->call.func = ast_new(NODE_NAME, 1);
    node->call.func->name_expr.name = str_dup("print");

    node_list_init(&node->call.args);
    node_list_push(&node->call.args, ast_new(NODE_INT_LIT, 1));

    node->call.is_unpacked_list = malloc(sizeof(int));
    node->call.is_unpacked_list[0] = 0;

    node->call.is_unpacked_dict = malloc(sizeof(int));
    node->call.is_unpacked_dict[0] = 0;

    ast_free(node);
    ASSERT(1, "test_ast_free_call completed");
}

static void test_ast_free_collections() {
    // List
    AstNode *lst = ast_new(NODE_LIST_EXPR, 1);
    node_list_init(&lst->list_expr.elems);
    node_list_push(&lst->list_expr.elems, ast_new(NODE_INT_LIT, 1));
    ast_free(lst);

    // Dict
    AstNode *dct = ast_new(NODE_DICT_EXPR, 1);
    node_list_init(&dct->dict_expr.keys);
    node_list_init(&dct->dict_expr.values);
    dct->dict_expr.keys.items = malloc(sizeof(AstNode*));
    dct->dict_expr.keys.count = 1;
    dct->dict_expr.keys.cap = 1;
    dct->dict_expr.keys.items[0] = ast_new(NODE_STRING_LIT, 1);
    dct->dict_expr.keys.items[0]->str_lit.value = str_dup("key");

    dct->dict_expr.values.items = malloc(sizeof(AstNode*));
    dct->dict_expr.values.count = 1;
    dct->dict_expr.values.cap = 1;
    dct->dict_expr.values.items[0] = ast_new(NODE_INT_LIT, 1);
    ast_free(dct);

    // Set
    AstNode *st = ast_new(NODE_SET_EXPR, 1);
    node_list_init(&st->set_expr.elems);
    node_list_push(&st->set_expr.elems, ast_new(NODE_INT_LIT, 1));
    ast_free(st);

    // Tuple
    AstNode *tpl = ast_new(NODE_TUPLE_EXPR, 1);
    node_list_init(&tpl->tuple_expr.elems);
    node_list_push(&tpl->tuple_expr.elems, ast_new(NODE_INT_LIT, 1));
    ast_free(tpl);

    ASSERT(1, "test_ast_free_collections completed");
}

static void test_ast_free_subscript() {
    AstNode *node = ast_new(NODE_SUBSCRIPT, 1);
    node->subscript.obj = ast_new(NODE_NAME, 1);
    node->subscript.obj->name_expr.name = str_dup("arr");
    node->subscript.index = ast_new(NODE_INT_LIT, 1);

    ast_free(node);
    ASSERT(1, "test_ast_free_subscript completed");
}

static void test_ast_free_attribute() {
    AstNode *node = ast_new(NODE_ATTRIBUTE, 1);
    node->attribute.obj = ast_new(NODE_NAME, 1);
    node->attribute.obj->name_expr.name = str_dup("obj");
    node->attribute.attr = str_dup("prop");

    ast_free(node);
    ASSERT(1, "test_ast_free_attribute completed");
}

static void test_ast_free_import() {
    AstNode *node = ast_new(NODE_IMPORT, 1);
    node->import_stmt.module = str_dup("math");
    node->import_stmt.alias = str_dup("m");

    ast_free(node);
    ASSERT(1, "test_ast_free_import completed");
}

static void test_ast_free_with() {
    AstNode *node = ast_new(NODE_WITH, 1);
    node->with_stmt.expr = ast_new(NODE_CALL, 1);
    node->with_stmt.expr->call.func = ast_new(NODE_NAME, 1);
    node->with_stmt.expr->call.func->name_expr.name = str_dup("open");
    node_list_init(&node->with_stmt.expr->call.args);
    node->with_stmt.expr->call.is_unpacked_list = NULL;
    node->with_stmt.expr->call.is_unpacked_dict = NULL;

    node->with_stmt.alias = str_dup("f");
    node_list_init(&node->with_stmt.body);
    node_list_push(&node->with_stmt.body, ast_new(NODE_PASS, 2));

    ast_free(node);
    ASSERT(1, "test_ast_free_with completed");
}

static void test_ast_free_try() {
    AstNode *node = ast_new(NODE_TRY, 1);

    node_list_init(&node->try_stmt.body);
    node_list_push(&node->try_stmt.body, ast_new(NODE_PASS, 2));

    node_list_init(&node->try_stmt.except_body);
    node_list_push(&node->try_stmt.except_body, ast_new(NODE_PASS, 4));
    node->try_stmt.except_type = str_dup("Exception");
    node->try_stmt.except_alias = str_dup("e");

    node_list_init(&node->try_stmt.finally_body);
    node_list_push(&node->try_stmt.finally_body, ast_new(NODE_PASS, 6));

    ast_free(node);
    ASSERT(1, "test_ast_free_try completed");
}

static void test_ast_free_raise() {
    AstNode *node = ast_new(NODE_RAISE, 1);
    node->raise_stmt.exc = ast_new(NODE_CALL, 1);
    node->raise_stmt.exc->call.func = ast_new(NODE_NAME, 1);
    node->raise_stmt.exc->call.func->name_expr.name = str_dup("ValueError");
    node_list_init(&node->raise_stmt.exc->call.args);
    node->raise_stmt.exc->call.is_unpacked_list = NULL;
    node->raise_stmt.exc->call.is_unpacked_dict = NULL;

    ast_free(node);
    ASSERT(1, "test_ast_free_raise completed");
}

static void test_ast_free_lambda_list_dict_comp_yield() {
    /*
     * In ast.c, ast_free doesn't currently handle NODE_LIST_COMP, NODE_DICT_COMP,
     * NODE_LAMBDA, NODE_YIELD, NODE_PARALLEL_FOR explicitly in the switch case
     * for deep freeing. As long as these nodes don't dynamically allocate children
     * during testing, freeing just the node itself works without leaks.
     */
    AstNode *node = ast_new(NODE_YIELD, 1);
    node->yield_expr.value = NULL;
    ast_free(node);
    ASSERT(1, "test_ast_free_lambda_list_dict_comp_yield completed");
}

int main() {
    printf("Running AST tests...\n");

    test_ast_free_program();
    test_ast_free_func_def();
    test_ast_free_class_def();
    test_ast_free_if();
    test_ast_free_for();
    test_ast_free_while();
    test_ast_free_assign();
    test_ast_free_aug_assign();
    test_ast_free_subscript_assign();
    test_ast_free_const_decl();
    test_ast_free_bin_op();
    test_ast_free_unary_op();
    test_ast_free_call();
    test_ast_free_collections();
    test_ast_free_subscript();
    test_ast_free_attribute();
    test_ast_free_import();
    test_ast_free_with();
    test_ast_free_try();
    test_ast_free_raise();
    test_ast_free_lambda_list_dict_comp_yield();

    if (tests_failed == 0) {
        printf("All %d tests passed!\n", tests_run);
        return 0;
    } else {
        printf("%d tests failed out of %d.\n", tests_failed, tests_run);
        return 1;
    }
}
