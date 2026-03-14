/*
 * LP Native String Module — Zero-Overhead C Implementation
 * Provides Python-like string methods compiled to native C.
 *
 * These are called as method-style: s.upper(), s.lower(), etc.
 * or via string module functions.
 */

#ifndef LP_NATIVE_STRINGS_H
#define LP_NATIVE_STRINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

/* str.upper() */
static inline const char *lp_str_upper(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    for (size_t i = 0; i < len; i++) r[i] = (char)toupper((unsigned char)s[i]);
    r[len] = '\0';
    return r;
}

/* str.lower() */
static inline const char *lp_str_lower(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    for (size_t i = 0; i < len; i++) r[i] = (char)tolower((unsigned char)s[i]);
    r[len] = '\0';
    return r;
}

/* str.strip() — remove leading/trailing whitespace */
static inline const char *lp_str_strip(const char *s) {
    if (!s || *s == '\0') {
        char *r = (char *)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }
    const char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    size_t slen = strlen(s);
    if (slen == 0) {
        char *r = (char *)malloc(1);
        if (r) r[0] = '\0';
        return r;
    }
    const char *end = s + slen - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    size_t len = end - start + 1;
    char *r = (char *)malloc(len + 1);
    if (r) {
        memcpy(r, start, len);
        r[len] = '\0';
    }
    return r;
}

/* str.lstrip() */
static inline const char *lp_str_lstrip(const char *s) {
    if (!s) return NULL;
    while (*s && isspace((unsigned char)*s)) s++;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    memcpy(r, s, len + 1);
    return r;
}

/* str.rstrip() */
static inline const char *lp_str_rstrip(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    memcpy(r, s, len);
    r[len] = '\0';
    return r;
}

/* str.startswith(prefix) */
static inline int lp_str_startswith(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    size_t plen = strlen(prefix);
    return (strncmp(s, prefix, plen) == 0);
}

/* str.endswith(suffix) */
static inline int lp_str_endswith(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t slen = strlen(s);
    size_t xlen = strlen(suffix);
    if (xlen > slen) return 0;
    return (strcmp(s + slen - xlen, suffix) == 0);
}

/* str.find(sub) — returns index or -1 */
static inline int64_t lp_str_find(const char *s, const char *sub) {
    if (!s || !sub) return -1;
    const char *p = strstr(s, sub);
    if (!p) return -1;
    return (int64_t)(p - s);
}

/* str.replace(old, new) */
static inline const char *lp_str_replace(const char *s, const char *old, const char *new_s) {
    if (!s) return NULL;
    size_t slen = strlen(s);
    size_t olen = old ? strlen(old) : 0;
    size_t nlen = new_s ? strlen(new_s) : 0;
    if (olen == 0) {
        char *r = (char *)malloc(slen + 1);
        if (!r) return NULL;
        memcpy(r, s, slen + 1);
        return r;
    }
    /* Count occurrences */
    int count = 0;
    const char *p = s;
    while ((p = strstr(p, old)) != NULL) { count++; p += olen; }
    /* Build result */
    size_t rlen = slen + count * ((int64_t)nlen - (int64_t)olen);
    char *r = (char *)malloc(rlen + 1);
    if (!r) return NULL;
    char *dst = r;
    p = s;
    while (*p) {
        if (strncmp(p, old, olen) == 0) {
            memcpy(dst, new_s, nlen);
            dst += nlen;
            p += olen;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    return r;
}

/* str.count(sub) */
static inline int64_t lp_str_count(const char *s, const char *sub) {
    if (!s || !sub) return 0;
    size_t sublen = strlen(sub);
    if (sublen == 0) return 0;
    int64_t count = 0;
    const char *p = s;
    while ((p = strstr(p, sub)) != NULL) { count++; p += sublen; }
    return count;
}

/* str.isdigit() */
static inline int lp_str_isdigit(const char *s) {
    if (!s || !*s) return 0;
    while (*s) { if (!isdigit((unsigned char)*s)) return 0; s++; }
    return 1;
}

/* str.isalpha() */
static inline int lp_str_isalpha(const char *s) {
    if (!s || !*s) return 0;
    while (*s) { if (!isalpha((unsigned char)*s)) return 0; s++; }
    return 1;
}

/* str.isalnum() */
static inline int lp_str_isalnum(const char *s) {
    if (!s || !*s) return 0;
    while (*s) { if (!isalnum((unsigned char)*s)) return 0; s++; }
    return 1;
}

/* len(str) */
static inline int64_t lp_str_len(const char *s) {
    if (!s) return 0;
    return (int64_t)strlen(s);
}

/* str.split(delimiter) — returns a simple array of strings */
typedef struct {
    const char **items;
    int64_t count;
    int64_t cap;
} LpStrArray;

static inline void lp_print_str_array(LpStrArray arr) {
    printf("[");
    for (int64_t i = 0; i < arr.count; i++) {
        if (i > 0) printf(", ");
        printf("'%s'", arr.items[i]);
    }
    printf("]\n");
}

static inline LpStrArray lp_str_split(const char *s, const char *delim) {
    LpStrArray arr;
    arr.cap = 16;
    arr.items = (const char **)malloc(arr.cap * sizeof(char *));
    arr.count = 0;

    if (!s) return arr;
    if (!arr.items) { arr.cap = 0; return arr; }

    const char *p = s;

    if (!delim || !*delim) {
        /* Split by any whitespace */
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            const char *start = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            size_t part_len = (size_t)(p - start);
            char *part = (char *)malloc(part_len + 1);
            if (!part) continue;
            memcpy(part, start, part_len);
            part[part_len] = '\0';
            if (arr.count >= arr.cap) {
                arr.cap *= 2;
                const char **new_items = (const char **)realloc(arr.items, arr.cap * sizeof(char *));
                if (!new_items) { free(part); continue; }
                arr.items = new_items;
            }
            arr.items[arr.count++] = part;
        }
        return arr;
    }

    size_t dlen = strlen(delim);
    while (*p) {
        const char *found = strstr(p, delim);
        size_t part_len = found ? (size_t)(found - p) : strlen(p);
        char *part = (char *)malloc(part_len + 1);
        if (!part) {
            if (!found) break;
            p = found + dlen;
            continue;
        }
        memcpy(part, p, part_len);
        part[part_len] = '\0';
        if (arr.count >= arr.cap) {
            arr.cap *= 2;
            const char **new_items = (const char **)realloc(arr.items, arr.cap * sizeof(char *));
            if (!new_items) { free(part); if (!found) break; p = found + dlen; continue; }
            arr.items = new_items;
        }
        arr.items[arr.count++] = part;
        if (!found) break;
        p = found + dlen;
    }
    return arr;
}

/* str.join(separator, array) */
static inline const char *lp_str_join(const char *sep, LpStrArray arr) {
    if (arr.count == 0 || !arr.items) {
        char *r = (char *)malloc(1);
        if (r) r[0] = '\0';
        return r ? r : "";
    }
    size_t seplen = sep ? strlen(sep) : 0;
    size_t total = 0;
    for (int64_t i = 0; i < arr.count; i++) {
        if (arr.items[i]) total += strlen(arr.items[i]);
        if (i > 0) total += seplen;
    }
    char *r = (char *)malloc(total + 1);
    if (!r) return "";
    char *dst = r;
    for (int64_t i = 0; i < arr.count; i++) {
        if (i > 0 && sep) { memcpy(dst, sep, seplen); dst += seplen; }
        if (arr.items[i]) {
            size_t len = strlen(arr.items[i]);
            memcpy(dst, arr.items[i], len);
            dst += len;
        }
    }
    *dst = '\0';
    return r;
}

#endif /* LP_NATIVE_STRINGS_H */
