#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"

void test_lexer_init() {
    Lexer lex;
    const char *source = "def foo(): pass";

    // Fill with garbage to ensure init actually sets the values
    memset(&lex, 0xFF, sizeof(Lexer));

    lexer_init(&lex, source);

    assert(lex.source == source);
    assert(lex.current == source);
    assert(lex.line == 1);
    assert(lex.indent_stack[0] == 0);
    assert(lex.indent_top == 0);
    assert(lex.pending_dedents == 0);
    assert(lex.at_line_start == 1);
    assert(lex.paren_depth == 0);
    assert(lex.eof_dedents_done == 0);

    printf("test_lexer_init passed\n");
}

int main() {
    test_lexer_init();
    printf("All lexer tests passed!\n");
    return 0;
}
