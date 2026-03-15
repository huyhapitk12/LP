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

/* Additional string methods */

/* str.center(width, fillchar) - Center string in width */
static inline const char *lp_str_center(const char *s, int width, char fill) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (width <= (int)len) {
        return strdup(s);
    }
    int pad = width - len;
    int left = pad / 2;
    int right = pad - left;
    char *r = (char *)malloc(width + 1);
    if (!r) return NULL;
    memset(r, fill, left);
    memcpy(r + left, s, len);
    memset(r + left + len, fill, right);
    r[width] = '\0';
    return r;
}

/* str.ljust(width, fillchar) - Left justify */
static inline const char *lp_str_ljust(const char *s, int width, char fill) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (width <= (int)len) {
        return strdup(s);
    }
    char *r = (char *)malloc(width + 1);
    if (!r) return NULL;
    memcpy(r, s, len);
    memset(r + len, fill, width - len);
    r[width] = '\0';
    return r;
}

/* str.rjust(width, fillchar) - Right justify */
static inline const char *lp_str_rjust(const char *s, int width, char fill) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (width <= (int)len) {
        return strdup(s);
    }
    char *r = (char *)malloc(width + 1);
    if (!r) return NULL;
    memset(r, fill, width - len);
    memcpy(r + width - len, s, len);
    r[width] = '\0';
    return r;
}

/* str.zfill(width) - Zero fill */
static inline const char *lp_str_zfill(const char *s, int width) {
    return lp_str_rjust(s, width, '0');
}

/* str.title() - Title case */
static inline const char *lp_str_title(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    int cap_next = 1;
    for (size_t i = 0; i < len; i++) {
        if (isspace((unsigned char)s[i]) || s[i] == '-' || s[i] == '_') {
            r[i] = s[i];
            cap_next = 1;
        } else if (cap_next) {
            r[i] = (char)toupper((unsigned char)s[i]);
            cap_next = 0;
        } else {
            r[i] = (char)tolower((unsigned char)s[i]);
        }
    }
    r[len] = '\0';
    return r;
}

/* str.capitalize() - Capitalize first char */
static inline const char *lp_str_capitalize(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    if (len > 0) {
        r[0] = (char)toupper((unsigned char)s[0]);
        for (size_t i = 1; i < len; i++) {
            r[i] = (char)tolower((unsigned char)s[i]);
        }
    }
    r[len] = '\0';
    return r;
}

/* str.swapcase() - Swap case */
static inline const char *lp_str_swapcase(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)malloc(len + 1);
    if (!r) return NULL;
    for (size_t i = 0; i < len; i++) {
        if (isupper((unsigned char)s[i])) r[i] = (char)tolower((unsigned char)s[i]);
        else if (islower((unsigned char)s[i])) r[i] = (char)toupper((unsigned char)s[i]);
        else r[i] = s[i];
    }
    r[len] = '\0';
    return r;
}

/* str.islower() - Check if all lowercase */
static inline int lp_str_islower(const char *s) {
    if (!s || !*s) return 0;
    int has_lower = 0;
    while (*s) {
        if (isupper((unsigned char)*s)) return 0;
        if (islower((unsigned char)*s)) has_lower = 1;
        s++;
    }
    return has_lower;
}

/* str.isupper() - Check if all uppercase */
static inline int lp_str_isupper(const char *s) {
    if (!s || !*s) return 0;
    int has_upper = 0;
    while (*s) {
        if (islower((unsigned char)*s)) return 0;
        if (isupper((unsigned char)*s)) has_upper = 1;
        s++;
    }
    return has_upper;
}

/* str.isspace() - Check if all whitespace */
static inline int lp_str_isspace(const char *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (!isspace((unsigned char)*s)) return 0;
        s++;
    }
    return 1;
}

/* str.rfind(sub) - Find from right */
static inline int64_t lp_str_rfind(const char *s, const char *sub) {
    if (!s || !sub) return -1;
    const char *last = NULL;
    const char *p = s;
    while ((p = strstr(p, sub)) != NULL) {
        last = p;
        p++;
    }
    return last ? (int64_t)(last - s) : -1;
}

/* str.rindex(sub) - Same as rfind but raises on error (returns -1 here) */
static inline int64_t lp_str_rindex(const char *s, const char *sub) {
    int64_t idx = lp_str_rfind(s, sub);
    return idx;
}

/* str.partition(sep) - Partition into 3 parts */
typedef struct {
    const char *part1;
    const char *part2;
    const char *part3;
} LpStrPartition;

static inline LpStrPartition lp_str_partition(const char *s, const char *sep) {
    LpStrPartition result = {NULL, NULL, NULL};
    if (!s) return result;
    if (!sep || !*sep) {
        result.part1 = strdup(s);
        result.part2 = strdup("");
        result.part3 = strdup("");
        return result;
    }
    const char *found = strstr(s, sep);
    if (found) {
        size_t len1 = found - s;
        char *p1 = (char *)malloc(len1 + 1);
        memcpy(p1, s, len1); p1[len1] = '\0';
        result.part1 = p1;
        result.part2 = strdup(sep);
        result.part3 = strdup(found + strlen(sep));
    } else {
        result.part1 = strdup(s);
        result.part2 = strdup("");
        result.part3 = strdup("");
    }
    return result;
}

#endif /* LP_NATIVE_STRINGS_H */
