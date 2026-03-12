#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../compiler/src/ast.h"

void test_node_list_push() {
    printf("Running test_node_list_push...\n");
    NodeList list;
    node_list_init(&list);

    // Initial state
    assert(list.count == 0);
    assert(list.cap == 0);
    assert(list.items == NULL);

    // Push first item
    AstNode *node1 = ast_new(NODE_INT_LIT, 1);
    node_list_push(&list, node1);

    assert(list.count == 1);
    assert(list.cap == 8); // Capacity should start at 8 according to ast.c
    assert(list.items != NULL);
    assert(list.items[0] == node1);

    // Push up to capacity
    for (int i = 1; i < 8; i++) {
        AstNode *node = ast_new(NODE_INT_LIT, i + 1);
        node_list_push(&list, node);
    }

    assert(list.count == 8);
    assert(list.cap == 8);

    // Push one more to trigger reallocation
    AstNode *node9 = ast_new(NODE_INT_LIT, 9);
    node_list_push(&list, node9);

    assert(list.count == 9);
    assert(list.cap == 16); // Capacity should double
    assert(list.items[8] == node9);

    // Cleanup
    for (int i = 0; i < list.count; i++) {
        ast_free(list.items[i]);
    }
    free(list.items);
    printf("test_node_list_push passed!\n");
}

int main() {
    test_node_list_push();
    return 0;
}
