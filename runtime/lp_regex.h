#ifndef LP_REGEX_H
#define LP_REGEX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

/* Simplified backtracking regex engine for LP */
/* Supported: ., *, +, ?, ^, $, \d, \w, \s, \D, \W, \S, [abc], [^abc] */

#define RE_MAX_PATTERN 128

typedef enum {
    RE_CHAR, RE_DOT, RE_BEGIN, RE_END,
    RE_DIGIT, RE_WORD, RE_SPACE,
    RE_NOT_DIGIT, RE_NOT_WORD, RE_NOT_SPACE,
    RE_CLASS, RE_NCLASS
} ReNodeType;

typedef struct {
    ReNodeType type;
    char ch;
    char class_chars[64];
    int class_len;
    int quantifier; /* 0=one, 1=*, 2=+, 3=? */
} ReNode;

typedef struct {
    ReNode nodes[RE_MAX_PATTERN];
    int count;
    int is_literal;
    int literal_len;
    char literal_str[256];
} RePattern;

/* ---- Pattern Cache (16-slot, hash-based) ---- */
#define RE_CACHE_SIZE 16
typedef struct {
    char pattern_str[256];
    RePattern compiled;
    int used;
} ReCacheEntry;

static ReCacheEntry re_cache[RE_CACHE_SIZE];

static inline uint32_t re_cache_hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) { h ^= (uint8_t)*s++; h *= 16777619u; }
    return h;
}

static inline RePattern* re_cache_lookup(const char* pattern) {
    uint32_t idx = re_cache_hash(pattern) & (RE_CACHE_SIZE - 1);
    if (re_cache[idx].used && strcmp(re_cache[idx].pattern_str, pattern) == 0)
        return &re_cache[idx].compiled;
    return NULL;
}

static inline void re_cache_store(const char* pattern, const RePattern* p) {
    uint32_t idx = re_cache_hash(pattern) & (RE_CACHE_SIZE - 1);
    strncpy(re_cache[idx].pattern_str, pattern, 255);
    re_cache[idx].pattern_str[255] = '\0';
    re_cache[idx].compiled = *p;
    re_cache[idx].used = 1;
}

/* ---- Compiler ---- */
static int re_compile(const char* pattern, RePattern* p) {
    p->count = 0;
    int i = 0;
    while (pattern[i]) {
        if (p->count >= RE_MAX_PATTERN) return 0;
        char c = pattern[i++];
        ReNode* n = &p->nodes[p->count++];
        n->quantifier = 0;
        n->class_len = 0;

        if (c == '^') { n->type = RE_BEGIN; }
        else if (c == '$') { n->type = RE_END; }
        else if (c == '.') { n->type = RE_DOT; }
        else if (c == '\\') {
            char nc = pattern[i++];
            if (nc == 'd') n->type = RE_DIGIT;
            else if (nc == 'w') n->type = RE_WORD;
            else if (nc == 's') n->type = RE_SPACE;
            else if (nc == 'D') n->type = RE_NOT_DIGIT;
            else if (nc == 'W') n->type = RE_NOT_WORD;
            else if (nc == 'S') n->type = RE_NOT_SPACE;
            else { n->type = RE_CHAR; n->ch = nc; }
        } else if (c == '[') {
            int negate = 0;
            if (pattern[i] == '^') { negate = 1; i++; }
            n->type = negate ? RE_NCLASS : RE_CLASS;
            n->class_len = 0;
            while (pattern[i] && pattern[i] != ']') {
                if (pattern[i+1] == '-' && pattern[i+2] && pattern[i+2] != ']') {
                    for (char r = pattern[i]; r <= pattern[i+2]; r++) {
                        if (n->class_len < 63) n->class_chars[n->class_len++] = r;
                    }
                    i += 3;
                } else {
                    if (n->class_len < 63) n->class_chars[n->class_len++] = pattern[i];
                    i++;
                }
            }
            n->class_chars[n->class_len] = '\0';
            if (pattern[i] == ']') i++;
        } else {
            n->type = RE_CHAR;
            n->ch = c;
        }

        /* Quantifiers */
        if (n->type != RE_BEGIN && n->type != RE_END) {
            if (pattern[i] == '*') { n->quantifier = 1; i++; }
            else if (pattern[i] == '+') { n->quantifier = 2; i++; }
            else if (pattern[i] == '?') { n->quantifier = 3; i++; }
        }
    }
    return 1;
}

/* Literal detection */
static inline void re_detect_literal(const char* pattern_str, RePattern* p) {
    p->is_literal = 1; p->literal_len = 0;
    int start = (pattern_str[0] == '^') ? 1 : 0;
    for (int i = start; pattern_str[i]; i++) {
        char c = pattern_str[i];
        if (c=='.'||c=='*'||c=='+'||c=='?'||c=='['||c=='\\'||c=='$'||c=='|'||c=='('||c==')') {
            p->is_literal = 0; return;
        }
        if (p->literal_len < 255) p->literal_str[p->literal_len++] = c;
    }
    p->literal_str[p->literal_len] = '\0';
}

static int re_compile_cached(const char* pattern_str, RePattern* p) {
    RePattern* cached = re_cache_lookup(pattern_str);
    if (cached) { *p = *cached; return 1; }
    if (!re_compile(pattern_str, p)) return 0;
    re_detect_literal(pattern_str, p);
    re_cache_store(pattern_str, p);
    return 1;
}

/* ---- Matching Engine ---- */
static int re_match_char(ReNode* n, unsigned char c) {
    switch (n->type) {
        case RE_CHAR: return c == n->ch;
        case RE_DOT: return c != '\n';
        case RE_DIGIT: return isdigit(c);
        case RE_WORD: return isalnum(c) || c == '_';
        case RE_SPACE: return isspace(c);
        case RE_NOT_DIGIT: return !isdigit(c);
        case RE_NOT_WORD: return !(isalnum(c) || c == '_');
        case RE_NOT_SPACE: return !isspace(c);
        case RE_CLASS:
            for (int i = 0; i < n->class_len; i++) if (n->class_chars[i] == c) return 1;
            return 0;
        case RE_NCLASS:
            for (int i = 0; i < n->class_len; i++) if (n->class_chars[i] == c) return 0;
            return 1;
        default: return 0;
    }
}

static int re_match_node(ReNode* nodes, int count, const char* text, int* match_len) {
    if (count == 0) { *match_len = 0; return 1; }
    ReNode* n = &nodes[0];
    if (n->type == RE_END) { *match_len = 0; return *text == '\0'; }

    if (n->quantifier == 1 || n->quantifier == 2) { /* * or + */
        int min_match = (n->quantifier == 2) ? 1 : 0;
        int i = 0;
        while (text[i] && re_match_char(n, (unsigned char)text[i])) i++;
        for (int j = i; j >= min_match; j--) {
            int sub_len = 0;
            if (re_match_node(nodes + 1, count - 1, text + j, &sub_len)) {
                *match_len = j + sub_len; return 1;
            }
        }
        return 0;
    }
    if (n->quantifier == 3) { /* ? */
        int sub_len = 0;
        if (text[0] && re_match_char(n, (unsigned char)text[0])) {
            if (re_match_node(nodes + 1, count - 1, text + 1, &sub_len)) {
                *match_len = 1 + sub_len; return 1;
            }
        }
        if (re_match_node(nodes + 1, count - 1, text, &sub_len)) {
            *match_len = sub_len; return 1;
        }
        return 0;
    }
    /* Single char */
    if (*text && re_match_char(n, (unsigned char)*text)) {
        int sub_len = 0;
        if (re_match_node(nodes + 1, count - 1, text + 1, &sub_len)) {
            *match_len = 1 + sub_len; return 1;
        }
    }
    return 0;
}

/* ---- Public API ---- */

static inline int lp_regex_is_match(const char* pattern, const char* text) {
    if (!pattern || !text) return 0;
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return 0;
    if (p.is_literal) return strstr(text, p.literal_str) != NULL;

    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    do {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len)) return 1;
        if (is_anchored) break;
    } while (*s++);
    return 0;
}

static inline const char* lp_regex_match(const char* pattern, const char* text) {
    if (!pattern || !text) return strdup("");
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return strdup("");
    if (p.is_literal) {
        const char* found = strstr(text, p.literal_str);
        if (found) { char* r=(char*)malloc(p.literal_len+1); memcpy(r,found,p.literal_len); r[p.literal_len]='\0'; return r; }
        return strdup("");
    }
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    do {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len)) {
            char* res = (char*)malloc(match_len + 1);
            strncpy(res, s, match_len); res[match_len] = '\0';
            return res;
        }
        if (is_anchored) break;
    } while (*s++);
    return strdup("");
}

static inline const char* lp_regex_search(const char* pattern, const char* text) {
    return lp_regex_match(pattern, text);
}

static inline const char* lp_regex_replace_first(const char* pattern, const char* text, const char* repl) {
    if (!pattern || !text || !repl) return strdup(text ? text : "");
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return strdup(text);
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    do {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len)) {
            int prefix_len = (int)(s - text);
            int repl_len = (int)strlen(repl);
            int rest_len = (int)strlen(s + match_len);
            char* res = (char*)malloc(prefix_len + repl_len + rest_len + 1);
            strncpy(res, text, prefix_len);
            strcpy(res + prefix_len, repl);
            strcpy(res + prefix_len + repl_len, s + match_len);
            return res;
        }
        if (is_anchored) break;
    } while (*s++);
    return strdup(text);
}

static inline const char* lp_regex_replace(const char* pattern, const char* text, const char* repl) {
    if (!pattern || !text || !repl) return strdup(text ? text : "");
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return strdup(text);
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    int cap = 1024, len = 0;
    char* res = (char*)malloc(cap);
    res[0] = '\0';
    while (*s) {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len)) {
            if (match_len == 0) {
                if (len + 2 > cap) { cap *= 2; res = (char*)realloc(res, cap); }
                res[len++] = *s++; continue;
            }
            int rlen = (int)strlen(repl);
            if (len + rlen + 2 > cap) { cap = (cap + rlen) * 2; res = (char*)realloc(res, cap); }
            strcpy(res + len, repl); len += rlen; s += match_len;
            if (is_anchored) break;
        } else {
            if (len + 2 > cap) { cap *= 2; res = (char*)realloc(res, cap); }
            res[len++] = *s++;
        }
    }
    res[len] = '\0';
    return res;
}

static inline int lp_regex_count(const char* pattern, const char* text) {
    if (!pattern || !text) return 0;
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return 0;
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    int count = 0;
    const char* s = text;
    while (*s) {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len)) {
            count++;
            s += (match_len > 0) ? match_len : 1;
            if (is_anchored) break;
        } else { s++; }
    }
    return count;
}

static inline const char* lp_regex_split(const char* pattern, const char* text) {
    if (!pattern || !text) return strdup("[]");
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return strdup("[]");
    int cap = 1024, len = 0;
    char* res = (char*)malloc(cap);
    res[len++] = '[';
    int first = 1;
    const char* s = text;
    const char* seg_start = text;
    while (*s) {
        int match_len = 0;
        if (re_match_node(p.nodes, p.count, s, &match_len) && match_len > 0) {
            int seg_len = (int)(s - seg_start);
            int needed = seg_len + 10;
            if (len + needed > cap) { cap = (cap + needed) * 2; res = (char*)realloc(res, cap); }
            if (!first) res[len++] = ','; res[len++] = ' ';
            res[len++] = '"';
            memcpy(res + len, seg_start, seg_len); len += seg_len;
            res[len++] = '"';
            first = 0;
            s += match_len;
            seg_start = s;
        } else { s++; }
    }
    /* Last segment */
    int seg_len = (int)(s - seg_start);
    int needed = seg_len + 10;
    if (len + needed > cap) { cap = (cap + needed) * 2; res = (char*)realloc(res, cap); }
    if (!first) res[len++] = ','; res[len++] = ' ';
    res[len++] = '"';
    memcpy(res + len, seg_start, seg_len); len += seg_len;
    res[len++] = '"';
    res[len++] = ']';
    res[len] = '\0';
    return res;
}

static inline const char* lp_regex_escape(const char* text) {
    if (!text) return strdup("");
    int tlen = (int)strlen(text);
    char* res = (char*)malloc(tlen * 2 + 1);
    int j = 0;
    for (int i = 0; i < tlen; i++) {
        char c = text[i];
        if (c=='.'||c=='*'||c=='+'||c=='?'||c=='^'||c=='$'||c=='['||c==']'||
            c=='('||c==')'||c=='{'||c=='}'||c=='|'||c=='\\') res[j++] = '\\';
        res[j++] = c;
    }
    res[j] = '\0';
    return res;
}

static inline int lp_regex_match_start(const char* pattern, const char* text) {
    if (!pattern || !text) return -1;
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return -1;
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    do {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len))
            return (int)(s - text);
        if (is_anchored) break;
    } while (*s++);
    return -1;
}

static inline int lp_regex_match_end(const char* pattern, const char* text) {
    if (!pattern || !text) return -1;
    RePattern p;
    if (!re_compile_cached(pattern, &p)) return -1;
    int is_anchored = (p.count > 0 && p.nodes[0].type == RE_BEGIN);
    const char* s = text;
    do {
        int match_len = 0;
        if (re_match_node(p.nodes + is_anchored, p.count - is_anchored, s, &match_len))
            return (int)(s - text) + match_len;
        if (is_anchored) break;
    } while (*s++);
    return -1;
}

#endif
