#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ast.h"

// Simple strdup for environments that might not have it or require extra macros
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *new_s = malloc(len);
    if (new_s) memcpy(new_s, s, len);
    return new_s;
}

void test_param_list_push() {
    printf("Testing param_list_push...\n");
    ParamList list;
    param_list_init(&list);

    assert(list.items == NULL);
    assert(list.count == 0);
    assert(list.capacity == 0);

    // Initial push should trigger growth to 8
    Param p1 = { .name = my_strdup("a"), .type_ann = NULL, .is_vararg = 0, .is_kwarg = 0 };
    param_list_push(&list, p1);
    assert(list.count == 1);
    assert(list.capacity == 8);
    assert(strcmp(list.items[0].name, "a") == 0);

    // Push more
    param_list_push(&list, (Param){ .name = my_strdup("b") });
    param_list_push(&list, (Param){ .name = my_strdup("c") });
    param_list_push(&list, (Param){ .name = my_strdup("d") });
    param_list_push(&list, (Param){ .name = my_strdup("e") });
    param_list_push(&list, (Param){ .name = my_strdup("f") });
    param_list_push(&list, (Param){ .name = my_strdup("g") });
    param_list_push(&list, (Param){ .name = my_strdup("h") });
    assert(list.count == 8);
    assert(list.capacity == 8);

    // Trigger growth to 16
    param_list_push(&list, (Param){ .name = my_strdup("i") });
    assert(list.count == 9);
    assert(list.capacity == 16);
    assert(strcmp(list.items[8].name, "i") == 0);

    // Clean up
    for(int i=0; i<list.count; i++) {
        free(list.items[i].name);
        if (list.items[i].type_ann) free(list.items[i].type_ann);
    }
    free(list.items);
    printf("param_list_push passed!\n");
}

void test_node_list_push() {
    printf("Testing node_list_push...\n");
    NodeList list;
    node_list_init(&list);

    assert(list.items == NULL);
    assert(list.count == 0);
    assert(list.cap == 0);

    for (int i = 0; i < 10; i++) {
        AstNode *node = ast_new(NODE_INT_LIT, i);
        node->int_lit.value = i;
        node_list_push(&list, node);
    }

    assert(list.count == 10);
    assert(list.cap == 16); // 8 -> 16

    for (int i = 0; i < 10; i++) {
        assert(list.items[i]->type == NODE_INT_LIT);
        assert(list.items[i]->int_lit.value == i);
        ast_free(list.items[i]);
    }
    free(list.items);
    printf("node_list_push passed!\n");
}

int main() {
    test_param_list_push();
    test_node_list_push();
    printf("All AST unit tests passed!\n");
    return 0;
}
