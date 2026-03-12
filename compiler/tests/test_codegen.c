#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

// Basic assertion macro
#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, msg); \
            exit(1); \
        } \
    } while (0)

static void test_buf_init() {
    Buffer b;
    buf_init(&b);
    ASSERT(b.data == NULL, "b.data should be NULL");
    ASSERT(b.len == 0, "b.len should be 0");
    ASSERT(b.cap == 0, "b.cap should be 0");
}

static void test_buf_write_basic() {
    Buffer b;
    buf_init(&b);

    buf_write(&b, "Hello");
    ASSERT(b.data != NULL, "b.data allocated");
    ASSERT(strcmp(b.data, "Hello") == 0, "content should be Hello");
    ASSERT(b.len == 5, "length should be 5");
    ASSERT(b.cap >= 6, "capacity should be at least 6");

    buf_write(&b, " World");
    ASSERT(strcmp(b.data, "Hello World") == 0, "content should be Hello World");
    ASSERT(b.len == 11, "length should be 11");

    buf_free(&b);
}

static void test_buf_write_edge_cases() {
    Buffer b;
    buf_init(&b);

    // Write empty string
    buf_write(&b, "");
    ASSERT(b.len == 0, "length should still be 0");

    // Write NULL
    buf_write(&b, NULL);
    ASSERT(b.len == 0, "length should still be 0");

    buf_write(&b, "a");
    ASSERT(b.len == 1, "length should be 1");
    ASSERT(strcmp(b.data, "a") == 0, "content should be 'a'");

    buf_free(&b);
}

static void test_buf_write_resize() {
    Buffer b;
    buf_init(&b);

    // Initial small writes
    buf_write(&b, "12345");
    int initial_cap = b.cap;

    // Trigger a resize by writing a very long string
    char long_str[200];
    memset(long_str, 'A', 199);
    long_str[199] = '\0';

    buf_write(&b, long_str);
    ASSERT(b.len == 204, "length should be 5 + 199 = 204");
    ASSERT(b.cap > initial_cap, "capacity should have grown");
    ASSERT(b.data[b.len] == '\0', "should be null terminated");

    buf_free(&b);
}

static void test_buf_printf() {
    Buffer b;
    buf_init(&b);

    buf_printf(&b, "Number: %d", 42);
    ASSERT(strcmp(b.data, "Number: 42") == 0, "buf_printf formats correctly");
    ASSERT(b.len == 10, "length should be 10");

    buf_free(&b);
}

int main() {
    test_buf_init();
    test_buf_write_basic();
    test_buf_write_edge_cases();
    test_buf_write_resize();
    test_buf_printf();

    printf("All Buffer tests passed.\n");
    return 0;
}
