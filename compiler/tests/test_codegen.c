#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "codegen.h"

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

void test_codegen_init() {
    CodeGen cg;

    // Fill with garbage to ensure init actually sets the values
    memset(&cg, 0xFF, sizeof(CodeGen));

    codegen_init(&cg);

    ASSERT(cg.header.data == NULL);
    ASSERT(cg.header.len == 0);
    ASSERT(cg.header.cap == 0);

    ASSERT(cg.helpers.data == NULL);
    ASSERT(cg.helpers.len == 0);
    ASSERT(cg.helpers.cap == 0);

    ASSERT(cg.funcs.data == NULL);
    ASSERT(cg.funcs.len == 0);
    ASSERT(cg.funcs.cap == 0);

    ASSERT(cg.main_body.data == NULL);
    ASSERT(cg.main_body.len == 0);
    ASSERT(cg.main_body.cap == 0);

    ASSERT(cg.scope != NULL);
    ASSERT(cg.scope->parent == NULL);
    ASSERT(cg.scope->count == 0);

    ASSERT(cg.had_error == 0);
    ASSERT(cg.error_msg[0] == '\0');
    ASSERT(cg.current_class == NULL);
    ASSERT(cg.import_count == 0);
    ASSERT(cg.uses_python == 0);
    ASSERT(cg.uses_native == 0);
    ASSERT(cg.uses_os == 0);
    ASSERT(cg.uses_sys == 0);
    ASSERT(cg.uses_strings == 0);
    ASSERT(cg.uses_http == 0);
    ASSERT(cg.uses_json == 0);
    ASSERT(cg.uses_sqlite == 0);
    ASSERT(cg.uses_thread == 0);
    ASSERT(cg.uses_memory == 0);
    ASSERT(cg.uses_platform == 0);
    ASSERT(cg.thread_adapter_count == 0);

    codegen_free(&cg);

    printf("test_codegen_init passed\n");
}

int main() {
    test_codegen_init();
    printf("All codegen tests passed!\n");
    return 0;
}
