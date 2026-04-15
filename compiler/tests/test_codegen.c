#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "codegen.h"
#include "parser.h"

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

/* --- Init tests (existing) --- */

void test_codegen_init() {
    CodeGen cg;

    memset(&cg, 0xFF, sizeof(CodeGen));

    codegen_init(&cg);

    ASSERT(cg.header.data == NULL);
    ASSERT(cg.header.len == 0);
    ASSERT(cg.header.cap == 0);
    ASSERT(cg.helpers.data == NULL);
    ASSERT(cg.funcs.data == NULL);
    ASSERT(cg.main_body.data == NULL);
    ASSERT(cg.scope != NULL);
    ASSERT(cg.scope->parent == NULL);
    ASSERT(cg.scope->count == 0);
    ASSERT(cg.had_error == 0);
    ASSERT(cg.error_msg[0] == '\0');
    ASSERT(cg.current_class == NULL);
    ASSERT(cg.import_count == 0);

    codegen_free(&cg);
    printf("  [PASS] test_codegen_init\n");
}

/* --- Behavioral tests --- */

/* Helper: parse source and generate C code */
static char *compile_to_c(const char *source, int *had_error) {
    LpArena *arena = lp_memory_arena_new(1024 * 1024);
    Parser parser;
    parser_init(&parser, source, arena);
    AstNode *program = parser_parse(&parser);

    if (parser.had_error) {
        *had_error = 1;
        ast_free(program);
        lp_memory_arena_free(arena);
        return NULL;
    }

    CodeGen cg;
    codegen_init(&cg);
    codegen_generate(&cg, program);
    char *output = codegen_get_output(&cg);

    *had_error = cg.had_error;

    codegen_free(&cg);
    ast_free(program);
    lp_memory_arena_free(arena);
    return output;
}

void test_codegen_print() {
    int err = 0;
    char *code = compile_to_c("print(42)\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    ASSERT(strstr(code, "printf") != NULL || strstr(code, "42") != NULL);

    free(code);
    printf("  [PASS] test_codegen_print\n");
}

void test_codegen_variable() {
    int err = 0;
    char *code = compile_to_c("x = 10\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    /* Generated C should contain the lp_ prefixed variable */
    ASSERT(strstr(code, "lp_x") != NULL);
    /* Should contain the value 10 */
    ASSERT(strstr(code, "10") != NULL);

    free(code);
    printf("  [PASS] test_codegen_variable\n");
}

void test_codegen_function() {
    int err = 0;
    char *code = compile_to_c("def foo():\n    pass\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    /* Generated C should contain the function name with lp_ prefix */
    ASSERT(strstr(code, "lp_foo") != NULL);

    free(code);
    printf("  [PASS] test_codegen_function\n");
}

void test_codegen_if() {
    int err = 0;
    char *code = compile_to_c("x = 1\nif x:\n    y = 2\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    ASSERT(strstr(code, "if") != NULL);
    ASSERT(strstr(code, "lp_x") != NULL);

    free(code);
    printf("  [PASS] test_codegen_if\n");
}

void test_codegen_for() {
    int err = 0;
    char *code = compile_to_c("for i in range(10):\n    pass\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    ASSERT(strstr(code, "for") != NULL || strstr(code, "lp_i") != NULL);

    free(code);
    printf("  [PASS] test_codegen_for\n");
}

void test_codegen_string() {
    int err = 0;
    char *code = compile_to_c("s = \"hello\"\n", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    ASSERT(strstr(code, "lp_s") != NULL);
    ASSERT(strstr(code, "hello") != NULL);

    free(code);
    printf("  [PASS] test_codegen_string\n");
}

void test_codegen_empty() {
    int err = 0;
    char *code = compile_to_c("", &err);

    ASSERT(err == 0);
    ASSERT(code != NULL);
    /* Even empty source should produce a valid C program with main() */
    ASSERT(strstr(code, "main") != NULL);

    free(code);
    printf("  [PASS] test_codegen_empty\n");
}

int main() {
    printf("Codegen Tests:\n");
    test_codegen_init();
    test_codegen_print();
    test_codegen_variable();
    test_codegen_function();
    test_codegen_if();
    test_codegen_for();
    test_codegen_string();
    test_codegen_empty();
    printf("All codegen tests passed!\n");
    return 0;
}
