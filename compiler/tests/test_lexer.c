#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lexer.h"

#define ASSERT(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

/* --- Init tests (existing) --- */

void test_lexer_init() {
    Lexer lex;
    const char *source = "def foo(): pass";

    // Fill with garbage to ensure init actually sets the values
    memset(&lex, 0xFF, sizeof(Lexer));

    lexer_init(&lex, source);

    ASSERT(lex.source == source);
    ASSERT(lex.current == source);
    ASSERT(lex.line == 1);
    ASSERT(lex.indent_stack[0] == 0);
    ASSERT(lex.indent_top == 0);
    ASSERT(lex.pending_dedents == 0);
    ASSERT(lex.at_line_start == 1);
    ASSERT(lex.paren_depth == 0);
    ASSERT(lex.eof_dedents_done == 0);

    printf("  [PASS] test_lexer_init\n");
}

/* --- Behavioral tests --- */

void test_lexer_keywords() {
    Lexer lex;
    lexer_init(&lex, "def class if elif else for while return import True False None pass break continue");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_DEF);
    t = lexer_next(&lex); ASSERT(t.type == TOK_CLASS);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IF);
    t = lexer_next(&lex); ASSERT(t.type == TOK_ELIF);
    t = lexer_next(&lex); ASSERT(t.type == TOK_ELSE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_FOR);
    t = lexer_next(&lex); ASSERT(t.type == TOK_WHILE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_RETURN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IMPORT);
    t = lexer_next(&lex); ASSERT(t.type == TOK_TRUE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_FALSE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_NONE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_PASS);
    t = lexer_next(&lex); ASSERT(t.type == TOK_BREAK);
    t = lexer_next(&lex); ASSERT(t.type == TOK_CONTINUE);

    printf("  [PASS] test_lexer_keywords\n");
}

void test_lexer_operators() {
    Lexer lex;
    lexer_init(&lex, "+ - * / == != >= <= > < = += -= **");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_PLUS);
    t = lexer_next(&lex); ASSERT(t.type == TOK_MINUS);
    t = lexer_next(&lex); ASSERT(t.type == TOK_STAR);
    t = lexer_next(&lex); ASSERT(t.type == TOK_SLASH);
    t = lexer_next(&lex); ASSERT(t.type == TOK_EQ);
    t = lexer_next(&lex); ASSERT(t.type == TOK_NEQ);
    t = lexer_next(&lex); ASSERT(t.type == TOK_GTE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_LTE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_GT);
    t = lexer_next(&lex); ASSERT(t.type == TOK_LT);
    t = lexer_next(&lex); ASSERT(t.type == TOK_ASSIGN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_PLUS_ASSIGN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_MINUS_ASSIGN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_DSTAR);

    printf("  [PASS] test_lexer_operators\n");
}

void test_lexer_numbers() {
    Lexer lex;
    lexer_init(&lex, "42 3.14 1e10 0");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_INT_LIT); ASSERT(t.int_val == 42);
    t = lexer_next(&lex); ASSERT(t.type == TOK_FLOAT_LIT); ASSERT(t.float_val == 3.14);
    t = lexer_next(&lex); ASSERT(t.type == TOK_FLOAT_LIT); /* 1e10 = float */
    t = lexer_next(&lex); ASSERT(t.type == TOK_INT_LIT); ASSERT(t.int_val == 0);

    printf("  [PASS] test_lexer_numbers\n");
}

void test_lexer_strings() {
    Lexer lex;
    lexer_init(&lex, "\"hello\" 'world'");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_STRING_LIT);
    ASSERT(strncmp(t.start, "hello", 5) == 0);
    t = lexer_next(&lex); ASSERT(t.type == TOK_STRING_LIT);
    ASSERT(strncmp(t.start, "world", 5) == 0);

    printf("  [PASS] test_lexer_strings\n");
}

void test_lexer_identifiers() {
    Lexer lex;
    lexer_init(&lex, "foo bar_baz _private x1");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_IDENTIFIER);
    ASSERT(t.length == 3); ASSERT(strncmp(t.start, "foo", 3) == 0);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IDENTIFIER);
    ASSERT(t.length == 7); ASSERT(strncmp(t.start, "bar_baz", 7) == 0);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IDENTIFIER);
    ASSERT(t.length == 8); ASSERT(strncmp(t.start, "_private", 8) == 0);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IDENTIFIER);
    ASSERT(t.length == 2); ASSERT(strncmp(t.start, "x1", 2) == 0);

    printf("  [PASS] test_lexer_identifiers\n");
}

void test_lexer_empty_input() {
    Lexer lex;
    lexer_init(&lex, "");

    Token t = lexer_next(&lex);
    ASSERT(t.type == TOK_EOF);

    printf("  [PASS] test_lexer_empty_input\n");
}

void test_lexer_comments() {
    Lexer lex;
    lexer_init(&lex, "42 # this is a comment");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_INT_LIT); ASSERT(t.int_val == 42);
    t = lexer_next(&lex); ASSERT(t.type == TOK_NEWLINE || t.type == TOK_EOF);

    printf("  [PASS] test_lexer_comments\n");
}

void test_lexer_indent_dedent() {
    Lexer lex;
    lexer_init(&lex, "if x:\n    pass\n");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_IF);
    t = lexer_next(&lex); ASSERT(t.type == TOK_IDENTIFIER); /* x */
    t = lexer_next(&lex); ASSERT(t.type == TOK_COLON);
    t = lexer_next(&lex); ASSERT(t.type == TOK_NEWLINE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_INDENT);
    t = lexer_next(&lex); ASSERT(t.type == TOK_PASS);
    t = lexer_next(&lex); ASSERT(t.type == TOK_NEWLINE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_DEDENT);

    printf("  [PASS] test_lexer_indent_dedent\n");
}

void test_lexer_delimiters() {
    Lexer lex;
    lexer_init(&lex, "( ) [ ] { } : , .");

    Token t;
    t = lexer_next(&lex); ASSERT(t.type == TOK_LPAREN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_RPAREN);
    t = lexer_next(&lex); ASSERT(t.type == TOK_LBRACKET);
    t = lexer_next(&lex); ASSERT(t.type == TOK_RBRACKET);
    t = lexer_next(&lex); ASSERT(t.type == TOK_LBRACE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_RBRACE);
    t = lexer_next(&lex); ASSERT(t.type == TOK_COLON);
    t = lexer_next(&lex); ASSERT(t.type == TOK_COMMA);
    t = lexer_next(&lex); ASSERT(t.type == TOK_DOT);

    printf("  [PASS] test_lexer_delimiters\n");
}

int main() {
    printf("Lexer Tests:\n");
    test_lexer_init();
    test_lexer_keywords();
    test_lexer_operators();
    test_lexer_numbers();
    test_lexer_strings();
    test_lexer_identifiers();
    test_lexer_empty_input();
    test_lexer_comments();
    test_lexer_indent_dedent();
    test_lexer_delimiters();
    printf("All lexer tests passed!\n");
    return 0;
}
