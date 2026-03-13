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

void test_parser_init() {
    Parser p;
    const char *source = "def foo(): pass";

    // Fill with garbage to ensure init actually sets the values
    memset(&p, 0xFF, sizeof(Parser));

    parser_init(&p, source);

    ASSERT(p.had_error == 0);
    ASSERT(p.error_msg[0] == '\0');

    // Lexer should be initialized
    ASSERT(p.lexer.source == source);

    // advance() should have been called, loading the first token
    // For "def foo(): pass", the first token is TOK_DEF
    ASSERT(p.current.type == TOK_DEF);
    ASSERT(strncmp(p.current.start, "def", 3) == 0);
    ASSERT(p.current.length == 3);

    printf("test_parser_init passed\n");
}

int main() {
    test_parser_init();
    printf("All parser tests passed!\n");
    return 0;
}
