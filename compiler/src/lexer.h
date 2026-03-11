#ifndef LP_LEXER_H
#define LP_LEXER_H

#include <stdint.h>

typedef enum {
    /* Literals */
    TOK_INT_LIT, TOK_FLOAT_LIT, TOK_STRING_LIT, TOK_IDENTIFIER,
    /* Keywords */
    TOK_DEF, TOK_CLASS, TOK_IF, TOK_ELIF, TOK_ELSE,
    TOK_FOR, TOK_WHILE, TOK_RETURN, TOK_IMPORT, TOK_FROM, TOK_AS,
    TOK_AND, TOK_OR, TOK_NOT, TOK_IN, TOK_IS,
    TOK_TRUE, TOK_FALSE, TOK_NONE,
    TOK_PASS, TOK_BREAK, TOK_CONTINUE,
    TOK_CONST, TOK_STRUCT, TOK_ASYNC, TOK_AWAIT,
    TOK_WITH, TOK_TRY, TOK_EXCEPT, TOK_FINALLY, TOK_RAISE,
    TOK_LAMBDA, TOK_YIELD, TOK_PARALLEL,
    TOK_PRIVATE, TOK_PROTECTED,
    /* Operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH,
    TOK_DSLASH, TOK_DSTAR, TOK_PERCENT,
    TOK_ASSIGN, TOK_EQ, TOK_NEQ,
    TOK_LT, TOK_GT, TOK_LTE, TOK_GTE,
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
    TOK_STAR_ASSIGN, TOK_SLASH_ASSIGN,
    TOK_ARROW, TOK_AT,
    /* Delimiters */
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET,
    TOK_LBRACE, TOK_RBRACE,
    TOK_COLON, TOK_COMMA, TOK_DOT,
    /* Structure */
    TOK_NEWLINE, TOK_INDENT, TOK_DEDENT,
    TOK_EOF, TOK_ERROR,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
    union {
        int64_t int_val;
        double float_val;
    };
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
} Token;

typedef struct {
    const char *source;
    const char *current;
    int line;
    int indent_stack[256];
    int indent_top;
    int pending_dedents;
    int at_line_start;
    int paren_depth;
    int eof_dedents_done;
} Lexer;

void lexer_init(Lexer *lex, const char *source);
Token lexer_next(Lexer *lex);
const char *token_type_name(TokenType type);

#endif
