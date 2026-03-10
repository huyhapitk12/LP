#include "lexer.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void lexer_init(Lexer *lex, const char *source) {
    lex->source = source;
    lex->current = source;
    lex->line = 1;
    lex->indent_stack[0] = 0;
    lex->indent_top = 0;
    lex->pending_dedents = 0;
    lex->at_line_start = 1;
    lex->paren_depth = 0;
    lex->eof_dedents_done = 0;
}

static Token make_token(TokenType type, const char *start, int len, int line) {
    Token t;
    memset(&t, 0, sizeof(Token));
    t.type = type;
    t.start = start;
    t.length = len;
    t.line = line;
    return t;
}

static int is_id_start(char c) { return isalpha((unsigned char)c) || c == '_'; }
static int is_id_char(char c) { return isalnum((unsigned char)c) || c == '_'; }

static void skip_newline_chars(Lexer *lex) {
    if (*lex->current == '\r') lex->current++;
    if (*lex->current == '\n') lex->current++;
    lex->line++;
}

/* Check if identifier is a keyword */
static TokenType check_keyword(const char *s, int len) {
    typedef struct { const char *kw; TokenType type; } KW;
    static const KW keywords[] = {
        {"def", TOK_DEF}, {"class", TOK_CLASS},
        {"if", TOK_IF}, {"elif", TOK_ELIF}, {"else", TOK_ELSE},
        {"for", TOK_FOR}, {"while", TOK_WHILE}, {"return", TOK_RETURN},
        {"import", TOK_IMPORT}, {"from", TOK_FROM}, {"as", TOK_AS},
        {"and", TOK_AND}, {"or", TOK_OR}, {"not", TOK_NOT},
        {"in", TOK_IN}, {"is", TOK_IS},
        {"True", TOK_TRUE}, {"False", TOK_FALSE}, {"None", TOK_NONE},
        {"pass", TOK_PASS}, {"break", TOK_BREAK}, {"continue", TOK_CONTINUE},
        {"const", TOK_CONST}, {"struct", TOK_STRUCT},
        {"async", TOK_ASYNC}, {"await", TOK_AWAIT},
        {"with", TOK_WITH},
        {"try", TOK_TRY}, {"except", TOK_EXCEPT},
        {"finally", TOK_FINALLY}, {"raise", TOK_RAISE},
        {"lambda", TOK_LAMBDA}, {"yield", TOK_YIELD},
        {"parallel", TOK_PARALLEL},
        {"private", TOK_PRIVATE}, {"protected", TOK_PROTECTED},
        {NULL, TOK_ERROR}
    };
    for (int i = 0; keywords[i].kw; i++) {
        if ((int)strlen(keywords[i].kw) == len && memcmp(s, keywords[i].kw, len) == 0)
            return keywords[i].type;
    }
    return TOK_IDENTIFIER;
}

/* Lex a single non-whitespace token */
static Token lex_token(Lexer *lex) {
    const char *start = lex->current;
    char c = *lex->current++;

    /* Identifiers and keywords */
    if (is_id_start(c)) {
        while (is_id_char(*lex->current)) lex->current++;
        int len = (int)(lex->current - start);
        TokenType kw = check_keyword(start, len);
        Token t = make_token(kw, start, len, lex->line);
        if (kw == TOK_TRUE) { t.int_val = 1; }
        else if (kw == TOK_FALSE) { t.int_val = 0; }
        return t;
    }

    /* Numbers */
    if (isdigit((unsigned char)c)) {
        int is_float = 0;
        while (isdigit((unsigned char)*lex->current)) lex->current++;
        if (*lex->current == '.' && isdigit((unsigned char)*(lex->current + 1))) {
            is_float = 1;
            lex->current++;
            while (isdigit((unsigned char)*lex->current)) lex->current++;
        }
        if (*lex->current == 'e' || *lex->current == 'E') {
            is_float = 1;
            lex->current++;
            if (*lex->current == '+' || *lex->current == '-') lex->current++;
            while (isdigit((unsigned char)*lex->current)) lex->current++;
        }
        int len = (int)(lex->current - start);
        Token t = make_token(is_float ? TOK_FLOAT_LIT : TOK_INT_LIT, start, len, lex->line);
        if (is_float) {
            char buf[64];
            int blen = len < 63 ? len : 63;
            memcpy(buf, start, blen); buf[blen] = '\0';
            t.float_val = atof(buf);
        } else {
            t.int_val = 0;
            for (const char *p = start; p < lex->current; p++)
                t.int_val = t.int_val * 10 + (*p - '0');
        }
        return t;
    }

    /* String literals */
    if (c == '"' || c == '\'') {
        char quote = c;
        /* Check for triple quotes */
        if (*lex->current == quote && *(lex->current + 1) == quote) {
            lex->current += 2;
            while (*lex->current) {
                if (*lex->current == quote && *(lex->current+1) == quote && *(lex->current+2) == quote) {
                    lex->current += 3;
                    return make_token(TOK_STRING_LIT, start + 3, (int)(lex->current - start - 6), lex->line);
                }
                if (*lex->current == '\n') lex->line++;
                lex->current++;
            }
        } else {
            while (*lex->current && *lex->current != quote && *lex->current != '\n') {
                if (*lex->current == '\\') lex->current++;
                lex->current++;
            }
            if (*lex->current == quote) lex->current++;
            return make_token(TOK_STRING_LIT, start + 1, (int)(lex->current - start - 2), lex->line);
        }
    }

    /* Two-character operators */
    switch (c) {
        case '+':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_PLUS_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_PLUS, start, 1, lex->line);
        case '-':
            if (*lex->current == '>') { lex->current++; return make_token(TOK_ARROW, start, 2, lex->line); }
            if (*lex->current == '=') { lex->current++; return make_token(TOK_MINUS_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_MINUS, start, 1, lex->line);
        case '*':
            if (*lex->current == '*') { lex->current++; return make_token(TOK_DSTAR, start, 2, lex->line); }
            if (*lex->current == '=') { lex->current++; return make_token(TOK_STAR_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_STAR, start, 1, lex->line);
        case '/':
            if (*lex->current == '/') { lex->current++; return make_token(TOK_DSLASH, start, 2, lex->line); }
            if (*lex->current == '=') { lex->current++; return make_token(TOK_SLASH_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_SLASH, start, 1, lex->line);
        case '%':
            return make_token(TOK_PERCENT, start, 1, lex->line);
        case '=':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_EQ, start, 2, lex->line); }
            return make_token(TOK_ASSIGN, start, 1, lex->line);
        case '!':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_NEQ, start, 2, lex->line); }
            return make_token(TOK_ERROR, start, 1, lex->line);
        case '<':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_LTE, start, 2, lex->line); }
            return make_token(TOK_LT, start, 1, lex->line);
        case '>':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_GTE, start, 2, lex->line); }
            return make_token(TOK_GT, start, 1, lex->line);
        case '(':
            lex->paren_depth++; return make_token(TOK_LPAREN, start, 1, lex->line);
        case ')':
            lex->paren_depth--; return make_token(TOK_RPAREN, start, 1, lex->line);
        case '[':
            lex->paren_depth++; return make_token(TOK_LBRACKET, start, 1, lex->line);
        case ']':
            lex->paren_depth--; return make_token(TOK_RBRACKET, start, 1, lex->line);
        case '{':
            lex->paren_depth++; return make_token(TOK_LBRACE, start, 1, lex->line);
        case '}':
            lex->paren_depth--; return make_token(TOK_RBRACE, start, 1, lex->line);
        case ':': return make_token(TOK_COLON, start, 1, lex->line);
        case ',': return make_token(TOK_COMMA, start, 1, lex->line);
        case '.': return make_token(TOK_DOT, start, 1, lex->line);
        case '@': return make_token(TOK_AT, start, 1, lex->line);
    }
    return make_token(TOK_ERROR, start, 1, lex->line);
}

Token lexer_next(Lexer *lex) {
    /* Phase 1: Emit pending dedents */
    if (lex->pending_dedents > 0) {
        lex->pending_dedents--;
        return make_token(TOK_DEDENT, lex->current, 0, lex->line);
    }

    /* Phase 2: Handle indentation at line start */
    while (lex->at_line_start) {
        lex->at_line_start = 0;
        int indent = 0;
        const char *p = lex->current;
        while (*p == ' ') { indent++; p++; }
        while (*p == '\t') { indent += 4; p++; }

        /* Skip blank lines and comment-only lines */
        if (*p == '\n' || *p == '\r') {
            lex->current = p;
            skip_newline_chars(lex);
            lex->at_line_start = 1;
            continue;
        }
        if (*p == '#') {
            while (*p && *p != '\n' && *p != '\r') p++;
            if (*p == '\n' || *p == '\r') {
                lex->current = p;
                skip_newline_chars(lex);
                lex->at_line_start = 1;
                continue;
            }
            /* Comment at EOF */
            lex->current = p;
            break;
        }
        if (*p == '\0') {
            lex->current = p;
            break;
        }

        lex->current = p;
        int cur_indent = lex->indent_stack[lex->indent_top];

        if (indent > cur_indent) {
            lex->indent_top++;
            lex->indent_stack[lex->indent_top] = indent;
            return make_token(TOK_INDENT, lex->current, 0, lex->line);
        } else if (indent < cur_indent) {
            while (lex->indent_top > 0 && lex->indent_stack[lex->indent_top] > indent) {
                lex->indent_top--;
                lex->pending_dedents++;
            }
            if (lex->pending_dedents > 0) {
                lex->pending_dedents--;
                return make_token(TOK_DEDENT, lex->current, 0, lex->line);
            }
        }
        /* Same indent: fall through to lex token */
    }

    /* Phase 3: Skip inline whitespace */
    while (*lex->current == ' ' || *lex->current == '\t') lex->current++;

    /* Phase 4: Skip comments */
    if (*lex->current == '#') {
        while (*lex->current && *lex->current != '\n' && *lex->current != '\r')
            lex->current++;
    }

    /* Phase 5: Handle newline */
    if (*lex->current == '\n' || *lex->current == '\r') {
        const char *nl = lex->current;
        skip_newline_chars(lex);
        if (lex->paren_depth == 0) {
            lex->at_line_start = 1;
            return make_token(TOK_NEWLINE, nl, 1, lex->line - 1);
        }
        return lexer_next(lex);
    }

    /* Phase 6: EOF — emit remaining dedents first */
    if (*lex->current == '\0') {
        if (!lex->eof_dedents_done && lex->indent_top > 0) {
            lex->eof_dedents_done = 1;
            lex->pending_dedents = lex->indent_top - 1;
            lex->indent_top = 0;
            return make_token(TOK_DEDENT, lex->current, 0, lex->line);
        }
        return make_token(TOK_EOF, lex->current, 0, lex->line);
    }

    /* Phase 7: Lex actual token */
    return lex_token(lex);
}

const char *token_type_name(TokenType type) {
    static const char *names[] = {
        "INT", "FLOAT", "STRING", "IDENT",
        "def", "class", "if", "elif", "else",
        "for", "while", "return", "import", "from", "as",
        "and", "or", "not", "in", "is",
        "True", "False", "None",
        "pass", "break", "continue",
        "const", "struct", "async", "await", "with",
        "try", "except", "finally", "raise",
        "lambda", "yield", "parallel",
        "private", "protected",
        "+", "-", "*", "/", "//", "**", "%",
        "=", "==", "!=", "<", ">", "<=", ">=",
        "+=", "-=", "*=", "/=", "->", "@",
        "(", ")", "[", "]", "{", "}",
        ":", ",", ".",
        "NEWLINE", "INDENT", "DEDENT",
        "EOF", "ERROR",
    };
    if (type >= 0 && type <= TOK_ERROR)
        return names[type];
    return "?";
}
