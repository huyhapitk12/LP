#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "codegen.h"

void test_buf_printf_basic() {
    Buffer b;
    buf_init(&b);
    buf_printf(&b, "Hello %s %d", "world", 42);
    assert(strcmp(b.data, "Hello world 42") == 0);
    assert(b.len == 14);
    buf_free(&b);
}

void test_buf_printf_empty() {
    Buffer b;
    buf_init(&b);
    buf_printf(&b, "");
    assert(strcmp(b.data, "") == 0);
    assert(b.len == 0);
    buf_free(&b);
}

void test_buf_printf_append() {
    Buffer b;
    buf_init(&b);
    buf_printf(&b, "A");
    buf_printf(&b, "B");
    buf_printf(&b, "C");
    assert(strcmp(b.data, "ABC") == 0);
    assert(b.len == 3);
    buf_free(&b);
}

void test_buf_printf_large() {
    Buffer b;
    buf_init(&b);
    char large_str[1024];
    memset(large_str, 'A', 1023);
    large_str[1023] = '\0';
    buf_printf(&b, "Prefix: %s", large_str);
    assert(strncmp(b.data, "Prefix: AAAA", 12) == 0);
    assert(b.len == 8 + 1023);
    assert(strlen(b.data) == (size_t)b.len);
    buf_free(&b);
}

void test_buf_printf_overflow() {
    Buffer b;
    buf_init(&b);
    char large_str[3000];
    memset(large_str, 'B', 2999);
    large_str[2999] = '\0';
    buf_printf(&b, "%s", large_str);
    assert(b.len == 2999);
    assert(strlen(b.data) == (size_t)2999);
    buf_free(&b);
}

void test_buf_printf_realloc() {
    Buffer b;
    buf_init(&b);
    // Write just enough to trigger a small allocation
    buf_printf(&b, "12345");
    // Write enough to force a realloc based on new logic
    char large_str[5000];
    memset(large_str, 'C', 4999);
    large_str[4999] = '\0';
    buf_printf(&b, "%s", large_str);
    assert(b.len == 5 + 4999);
    assert(strncmp(b.data, "12345CCCC", 9) == 0);
    assert(strlen(b.data) == (size_t)5004);
    buf_free(&b);
}

int main() {
    printf("Running buf_printf tests...\n");
    test_buf_printf_basic();
    test_buf_printf_empty();
    test_buf_printf_append();
    test_buf_printf_large();
    test_buf_printf_overflow();
    test_buf_printf_realloc();
    printf("All codegen tests passed!\n");
    return 0;
}
