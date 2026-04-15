#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

/* --- Init tests (existing) --- */

void test_parser_init() {
    Parser p;
    const char *source = "def foo(): pass";

    memset(&p, 0xFF, sizeof(Parser));

    LpArena *arena = lp_memory_arena_new(1024);
    parser_init(&p, source, arena);

    ASSERT(p.had_error == 0);
    ASSERT(p.error_msg[0] == '\0');
    ASSERT(p.lexer.source == source);
    ASSERT(p.current.type == TOK_DEF);
    ASSERT(strncmp(p.current.start, "def", 3) == 0);
    ASSERT(p.current.length == 3);

    lp_memory_arena_free(arena);
    printf("  [PASS] test_parser_init\n");
}

/* --- Behavioral tests --- */

void test_parse_empty() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->type == NODE_PROGRAM);
    ASSERT(program->program.stmts.count == 0);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_empty\n");
}

void test_parse_assign() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "x = 42\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->type == NODE_PROGRAM);
    ASSERT(program->program.stmts.count == 1);

    AstNode *stmt = program->program.stmts.items[0];
    ASSERT(stmt->type == NODE_ASSIGN);
    ASSERT(strcmp(stmt->assign.name, "x") == 0);
    ASSERT(stmt->assign.value != NULL);
    ASSERT(stmt->assign.value->type == NODE_INT_LIT);
    ASSERT(stmt->assign.value->int_lit.value == 42);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_assign\n");
}

void test_parse_func_def() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "def foo():\n    pass\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *func = program->program.stmts.items[0];
    ASSERT(func->type == NODE_FUNC_DEF);
    ASSERT(strcmp(func->func_def.name, "foo") == 0);
    ASSERT(func->func_def.params.count == 0);
    ASSERT(func->func_def.body.count == 1);
    ASSERT(func->func_def.body.items[0]->type == NODE_PASS);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_func_def\n");
}

void test_parse_if() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "if x:\n    pass\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *if_stmt = program->program.stmts.items[0];
    ASSERT(if_stmt->type == NODE_IF);
    ASSERT(if_stmt->if_stmt.cond != NULL);
    ASSERT(if_stmt->if_stmt.cond->type == NODE_NAME);
    ASSERT(if_stmt->if_stmt.then_body.count == 1);
    ASSERT(if_stmt->if_stmt.else_branch == NULL);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_if\n");
}

void test_parse_for() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "for i in range(10):\n    pass\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *for_stmt = program->program.stmts.items[0];
    ASSERT(for_stmt->type == NODE_FOR);
    ASSERT(strcmp(for_stmt->for_stmt.var, "i") == 0);
    ASSERT(for_stmt->for_stmt.iter != NULL);
    ASSERT(for_stmt->for_stmt.body.count == 1);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_for\n");
}

void test_parse_class() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "class Foo:\n    pass\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *cls = program->program.stmts.items[0];
    ASSERT(cls->type == NODE_CLASS_DEF);
    ASSERT(strcmp(cls->class_def.name, "Foo") == 0);
    ASSERT(cls->class_def.base_class == NULL);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_class\n");
}

void test_parse_error() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    /* Missing condition for 'if' should cause a parse error */
    parser_init(&p, "if :\n    pass\n", arena);
    AstNode *program = parser_parse(&p);

    /* Parser should have set had_error */
    ASSERT(p.had_error == 1);
    ASSERT(p.error_msg[0] != '\0');

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_error\n");
}

void test_parse_import() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "import math\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *imp = program->program.stmts.items[0];
    ASSERT(imp->type == NODE_IMPORT);
    ASSERT(strcmp(imp->import_stmt.module, "math") == 0);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_import\n");
}

void test_parse_return() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "def f():\n    return 42\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *func = program->program.stmts.items[0];
    ASSERT(func->type == NODE_FUNC_DEF);
    ASSERT(func->func_def.body.count == 1);
    AstNode *ret = func->func_def.body.items[0];
    ASSERT(ret->type == NODE_RETURN);
    ASSERT(ret->return_stmt.value != NULL);
    ASSERT(ret->return_stmt.value->type == NODE_INT_LIT);
    ASSERT(ret->return_stmt.value->int_lit.value == 42);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_return\n");
}

void test_parse_expression() {
    LpArena *arena = lp_memory_arena_new(1024 * 64);
    Parser p;
    parser_init(&p, "x = 1 + 2 * 3\n", arena);
    AstNode *program = parser_parse(&p);

    ASSERT(p.had_error == 0);
    ASSERT(program != NULL);
    ASSERT(program->program.stmts.count == 1);

    AstNode *stmt = program->program.stmts.items[0];
    ASSERT(stmt->type == NODE_ASSIGN);
    /* Value should be BIN_OP(+, 1, BIN_OP(*, 2, 3)) due to precedence */
    AstNode *val = stmt->assign.value;
    ASSERT(val->type == NODE_BIN_OP);
    ASSERT(val->bin_op.op == TOK_PLUS);
    ASSERT(val->bin_op.left->type == NODE_INT_LIT);
    ASSERT(val->bin_op.left->int_lit.value == 1);
    ASSERT(val->bin_op.right->type == NODE_BIN_OP);
    ASSERT(val->bin_op.right->bin_op.op == TOK_STAR);

    ast_free(program);
    lp_memory_arena_free(arena);
    printf("  [PASS] test_parse_expression\n");
}

int main() {
    printf("Parser Tests:\n");
    test_parser_init();
    test_parse_empty();
    test_parse_assign();
    test_parse_func_def();
    test_parse_if();
    test_parse_for();
    test_parse_class();
    test_parse_error();
    test_parse_import();
    test_parse_return();
    test_parse_expression();
    printf("All parser tests passed!\n");
    return 0;
}
