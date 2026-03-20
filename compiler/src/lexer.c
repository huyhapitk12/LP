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
    switch (len) {
        case 2:
            if (s[0] == 'i' && s[1] == 'f') return TOK_IF;
            if (s[0] == 'i' && s[1] == 'n') return TOK_IN;
            if (s[0] == 'i' && s[1] == 's') return TOK_IS;
            if (s[0] == 'o' && s[1] == 'r') return TOK_OR;
            if (s[0] == 'a' && s[1] == 's') return TOK_AS;
            break;
        case 3:
            if (memcmp(s, "def", 3) == 0) return TOK_DEF;
            if (memcmp(s, "for", 3) == 0) return TOK_FOR;
            if (memcmp(s, "and", 3) == 0) return TOK_AND;
            if (memcmp(s, "not", 3) == 0) return TOK_NOT;
            if (memcmp(s, "try", 3) == 0) return TOK_TRY;
            if (memcmp(s, "gpu", 3) == 0) return TOK_GPU;
            if (memcmp(s, "cpu", 3) == 0) return TOK_CPU;
            if (memcmp(s, "xss", 3) == 0) return TOK_XSS;
            break;
        case 4:
            if (memcmp(s, "elif", 4) == 0) return TOK_ELIF;
            if (memcmp(s, "else", 4) == 0) return TOK_ELSE;
            if (memcmp(s, "from", 4) == 0) return TOK_FROM;
            if (memcmp(s, "True", 4) == 0) return TOK_TRUE;
            if (memcmp(s, "true", 4) == 0) return TOK_TRUE;
            if (memcmp(s, "None", 4) == 0) return TOK_NONE;
            if (memcmp(s, "none", 4) == 0) return TOK_NONE;
            if (memcmp(s, "pass", 4) == 0) return TOK_PASS;
            if (memcmp(s, "with", 4) == 0) return TOK_WITH;
            if (memcmp(s, "case", 4) == 0) return TOK_CASE;
            if (memcmp(s, "auth", 4) == 0) return TOK_AUTH;
            if (memcmp(s, "rate", 4) == 0) return TOK_RATE;
            if (memcmp(s, "hash", 4) == 0) return TOK_HASH;
            if (memcmp(s, "csrf", 4) == 0) return TOK_CSRF;
            if (memcmp(s, "cors", 4) == 0) return TOK_CORS;
            if (memcmp(s, "user", 4) == 0) return TOK_USER;
            break;
        case 5:
            if (memcmp(s, "class", 5) == 0) return TOK_CLASS;
            if (memcmp(s, "while", 5) == 0) return TOK_WHILE;
            if (memcmp(s, "False", 5) == 0) return TOK_FALSE;
            if (memcmp(s, "false", 5) == 0) return TOK_FALSE;
            if (memcmp(s, "break", 5) == 0) return TOK_BREAK;
            if (memcmp(s, "const", 5) == 0) return TOK_CONST;
            if (memcmp(s, "async", 5) == 0) return TOK_ASYNC;
            if (memcmp(s, "await", 5) == 0) return TOK_AWAIT;
            if (memcmp(s, "raise", 5) == 0) return TOK_RAISE;
            if (memcmp(s, "yield", 5) == 0) return TOK_YIELD;
            if (memcmp(s, "match", 5) == 0) return TOK_MATCH;
            if (memcmp(s, "chunk", 5) == 0) return TOK_CHUNK;
            if (memcmp(s, "level", 5) == 0) return TOK_LEVEL;
            if (memcmp(s, "limit", 5) == 0) return TOK_LIMIT;
            if (memcmp(s, "write", 5) == 0) return TOK_WRITE;
            if (memcmp(s, "admin", 5) == 0) return TOK_ADMIN;
            if (memcmp(s, "guest", 5) == 0) return TOK_GUEST;
            break;
        case 6:
            if (memcmp(s, "return", 6) == 0) return TOK_RETURN;
            if (memcmp(s, "import", 6) == 0) return TOK_IMPORT;
            if (memcmp(s, "struct", 6) == 0) return TOK_STRUCT;
            if (memcmp(s, "except", 6) == 0) return TOK_EXCEPT;
            if (memcmp(s, "lambda", 6) == 0) return TOK_LAMBDA;
            if (memcmp(s, "device", 6) == 0) return TOK_DEVICE;
            break;
        case 7:
            if (memcmp(s, "finally", 7) == 0) return TOK_FINALLY;
            if (memcmp(s, "private", 7) == 0) return TOK_PRIVATE;
            if (memcmp(s, "threads", 7) == 0) return TOK_THREADS;
            if (memcmp(s, "unified", 7) == 0) return TOK_UNIFIED;
            if (memcmp(s, "encrypt", 7) == 0) return TOK_ENCRYPT;
            if (memcmp(s, "headers", 7) == 0) return TOK_HEADERS;
            break;
        case 8:
            if (memcmp(s, "continue", 8) == 0) return TOK_CONTINUE;
            if (memcmp(s, "parallel", 8) == 0) return TOK_PARALLEL;
            if (memcmp(s, "settings", 8) == 0) return TOK_SETTINGS;
            if (memcmp(s, "schedule", 8) == 0) return TOK_SCHEDULE;
            if (memcmp(s, "security", 8) == 0) return TOK_SECURITY;
            if (memcmp(s, "validate", 8) == 0) return TOK_VALIDATE;
            if (memcmp(s, "sanitize", 8) == 0) return TOK_SANITIZE;
            if (memcmp(s, "readonly", 8) == 0) return TOK_READONLY;
            break;
        case 9:
            if (memcmp(s, "protected", 9) == 0) return TOK_PROTECTED;
            if (memcmp(s, "injection", 9) == 0) return TOK_INJECTION;
            break;
    }
    return TOK_IDENTIFIER;
}

/* Lex a single non-whitespace token */
static Token lex_token(Lexer *lex) {
    const char *start = lex->current;
    char c = *lex->current++;

    /* F-Strings: f"..." or f'...' */
    if (c == 'f' && (*lex->current == '"' || *lex->current == '\'')) {
        char quote = *lex->current++;
        while (*lex->current && *lex->current != quote && *lex->current != '\n') {
            if (*lex->current == '\\') lex->current++;
            lex->current++;
        }
        if (*lex->current == quote) lex->current++;
        /* Return f-string content (without f and quotes) */
        return make_token(TOK_FSTRING_LIT, start + 2, (int)(lex->current - start - 3), lex->line);
    }

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
            if (*lex->current == '<') {
                lex->current++;
                if (*lex->current == '=') { lex->current++; return make_token(TOK_LSHIFT_ASSIGN, start, 3, lex->line); }
                return make_token(TOK_LSHIFT, start, 2, lex->line);
            }
            if (*lex->current == '=') { lex->current++; return make_token(TOK_LTE, start, 2, lex->line); }
            return make_token(TOK_LT, start, 1, lex->line);
        case '>':
            if (*lex->current == '>') {
                lex->current++;
                if (*lex->current == '=') { lex->current++; return make_token(TOK_RSHIFT_ASSIGN, start, 3, lex->line); }
                return make_token(TOK_RSHIFT, start, 2, lex->line);
            }
            if (*lex->current == '=') { lex->current++; return make_token(TOK_GTE, start, 2, lex->line); }
            return make_token(TOK_GT, start, 1, lex->line);
        case '&':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_BIT_AND_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_BIT_AND, start, 1, lex->line);
        case '|':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_BIT_OR_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_BIT_OR, start, 1, lex->line);
        case '^':
            if (*lex->current == '=') { lex->current++; return make_token(TOK_BIT_XOR_ASSIGN, start, 2, lex->line); }
            return make_token(TOK_BIT_XOR, start, 1, lex->line);
        case '~':
            return make_token(TOK_BIT_NOT, start, 1, lex->line);
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
        "INT", "FLOAT", "STRING", "FSTRING", "IDENT",
        "def", "class", "if", "elif", "else",
        "for", "while", "return", "import", "from", "as",
        "and", "or", "not", "in", "is",
        "True", "False", "None",
        "pass", "break", "continue",
        "const", "struct", "async", "await",
        "with", "try", "except", "finally", "raise",
        "lambda", "yield", "parallel",
        "private", "protected",
        /* Pattern Matching */
        "match", "case",
        /* Settings/Pragma keywords */
        "settings", "gpu", "cpu", "device", "threads",
        "schedule", "chunk", "unified",
        /* Security keywords */
        "security", "level", "auth", "rate", "limit",
        "validate", "sanitize", "encrypt", "hash",
        "injection", "xss", "csrf", "cors", "headers",
        "readonly", "write", "admin", "user", "guest",
        "+", "-", "*", "/", "//", "**", "%",
        "&", "|", "^", "~", "<<", ">>",
        "=", "==", "!=", "<", ">", "<=", ">=",
        "+=", "-=", "*=", "/=",
        "&=", "|=", "^=", "<<=", ">>=",
        "->", "@",
        "(", ")", "[", "]", "{", "}",
        ":", ",", ".",
        "NEWLINE", "INDENT", "DEDENT",
        "EOF", "ERROR",
    };
    if (type >= 0 && type <= TOK_ERROR)
        return names[type];
    return "?";
}
