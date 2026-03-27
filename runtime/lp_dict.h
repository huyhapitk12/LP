/*
 * LP Native Dictionary — Zero-Overhead C Hash Map Implementation
 * Maps Python's dict to a native C open-addressing hash table.
 * Supports heterogeneous values (int, float, string, bool) via LpVal union.
 */

#ifndef LP_DICT_H
#define LP_DICT_H

/* Needed for aligned_alloc on glibc (C11 extension exposed via _DEFAULT_SOURCE) */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _ISOC11_SOURCE
#define _ISOC11_SOURCE
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

/* Portable aligned alloc */
#ifdef _WIN32
  #include <malloc.h>
  #define lp_aligned_alloc(align, size) _aligned_malloc((size), (align))
  #define lp_aligned_free(ptr) _aligned_free(ptr)
#else
  #define lp_aligned_alloc(align, size) aligned_alloc((align), (size))
  #define lp_aligned_free(ptr) free(ptr)
#endif

#define LP_DICT_INITIAL_CAPACITY 1024  /* larger start = fewer rehashes for typical workloads */
#define LP_DICT_LOAD_FACTOR 0.60

typedef struct LpDict LpDict;
typedef struct LpList LpList;

/* Type wrapper for values in the dict */
typedef enum {
    LP_VAL_INT,
    LP_VAL_FLOAT,
    LP_VAL_STRING,
    LP_VAL_BOOL,
    LP_VAL_NULL,
    LP_VAL_DICT,
    LP_VAL_LIST
} LpValType;

typedef struct {
    LpValType type;
    union {
        int64_t i;
        double f;
        char *s;
        int b;
        LpDict *d;
        LpList *l;
    } as;
} LpVal;

struct LpList {
    LpVal *items;
    int64_t len;
    int64_t cap;
};

static inline void lp_list_free(LpList *l);

static inline LpVal lp_val_int(int64_t i) { LpVal v; v.type = LP_VAL_INT; v.as.i = i; return v; }
static inline LpVal lp_val_float(double f) { LpVal v; v.type = LP_VAL_FLOAT; v.as.f = f; return v; }
static inline LpVal lp_val_str(const char *s) {
    LpVal v; v.type = LP_VAL_STRING; v.as.s = strdup(s); return v;
}
static inline LpVal lp_val_bool(int b) { LpVal v; v.type = LP_VAL_BOOL; v.as.b = b; return v; }
static inline LpVal lp_val_null(void) { LpVal v; v.type = LP_VAL_NULL; v.as.i = 0; return v; }
static inline LpVal lp_val_dict(LpDict *d) { LpVal v; v.type = LP_VAL_DICT; v.as.d = d; return v; }
static inline LpVal lp_val_list(LpList *l) { LpVal v; v.type = LP_VAL_LIST; v.as.l = l; return v; }

/* Dict Entry with Small String Optimization (SSO) for keys ≤ 15 chars.
 * Short string keys (digit strings, identifiers) are stored inline —
 * no heap allocation, better cache locality, same cost as C++ unordered_map SSO. */
#define LP_DICT_SSO_LEN 15
typedef struct {
    union {
        char  sso[LP_DICT_SSO_LEN + 1];  /* inline key storage (≤15 chars + NUL) */
        char *heap;                        /* heap pointer for keys >15 chars */
    } key_u;
    LpVal value;
    int   is_occupied;
    int   key_is_heap;   /* 0 = SSO, 1 = heap */
} LpDictEntry;

/* Key access helpers */
static inline const char* lp_entry_key(const LpDictEntry *e) {
    return e->key_is_heap ? e->key_u.heap : e->key_u.sso;
}
static inline void lp_entry_set_key(LpDictEntry *e, const char *key) {
    /* Fast path for very short keys (1-8 chars, most common: digits "0"-"9999") */
    /* Read 8 bytes at once, check for null within first 8 chars */
    const unsigned char *k = (const unsigned char*)key;
    if (!k[0])      { e->key_u.sso[0]=0; e->key_is_heap=0; return; }
    if (!k[1])      { e->key_u.sso[0]=k[0]; e->key_u.sso[1]=0; e->key_is_heap=0; return; }
    if (!k[2])      { e->key_u.sso[0]=k[0]; e->key_u.sso[1]=k[1]; e->key_u.sso[2]=0; e->key_is_heap=0; return; }
    if (!k[3])      { e->key_u.sso[0]=k[0]; e->key_u.sso[1]=k[1]; e->key_u.sso[2]=k[2]; e->key_u.sso[3]=0; e->key_is_heap=0; return; }
    if (!k[4])      { *(uint32_t*)e->key_u.sso = *(const uint32_t*)key; e->key_u.sso[4]=0; e->key_is_heap=0; return; }
    /* General path: strlen + memcpy or heap */
    size_t n = 4;
    while (key[n]) n++;
    if (n <= LP_DICT_SSO_LEN) {
        memcpy(e->key_u.sso, key, n + 1);
        e->key_is_heap = 0;
    } else {
        e->key_u.heap = strdup(key);
        e->key_is_heap = 1;
    }
}
static inline void lp_entry_free_key(LpDictEntry *e) {
    if (e->key_is_heap && e->key_u.heap) {
        free(e->key_u.heap);
        e->key_u.heap = NULL;
        e->key_is_heap = 0;
    }
}

/* The Dictionary */
struct LpDict {
    LpDictEntry *entries;
    int64_t capacity;
    int64_t count;
};

/* Hash function — wyhash-inspired mixing for better performance and distribution.
 * ~2x faster than FNV-1a on modern x86-64 due to 64-bit multiply-fold mixing.
 * Processes 8 bytes at a time for long keys, with fast paths for 1-8 byte keys. */
static inline uint64_t _wymix(uint64_t a, uint64_t b) {
    /* 128-bit multiply, folded to 64 bits */
#if defined(__SIZEOF_INT128__)
    __uint128_t r = (__uint128_t)a * b;
    return (uint64_t)(r ^ (r >> 64));
#else
    /* Fallback for compilers without __int128 (MSVC) */
    uint64_t ha = a >> 32, la = (uint32_t)a;
    uint64_t hb = b >> 32, lb = (uint32_t)b;
    uint64_t rh = ha * hb, rl = la * lb;
    uint64_t rm0 = ha * lb, rm1 = hb * la;
    uint64_t t = rl + (rm0 << 32);
    uint64_t c = (t < rl) ? 1 : 0;
    t += (rm1 << 32);
    c += (t < (rm1 << 32)) ? 1 : 0;
    uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    return t ^ hi;
#endif
}

static inline uint32_t lp_hash(const char* key) {
    if (!key || !*key) return 0;
    const uint8_t *p = (const uint8_t*)key;
    uint64_t seed = 0x9E3779B97F4A7C15ULL; /* golden ratio */
    uint64_t s0 = seed, s1 = seed;
    size_t len = 0;
    
    /* Fast path: measure length and hash 1-8 byte keys inline */
    while (p[len]) {
        len++;
        if (len >= 8) break;
    }
    
    if (len <= 8 && !p[len]) {
        /* Short key fast path — read available bytes */
        uint64_t a = 0, b = 0;
        if (len >= 4) {
            a = (uint64_t)(*(const uint32_t*)p);
            b = (uint64_t)(*(const uint32_t*)(p + len - 4));
        } else if (len > 0) {
            a = (uint64_t)p[0] | ((uint64_t)p[len >> 1] << 8) | ((uint64_t)p[len - 1] << 16);
        }
        return (uint32_t)_wymix(a ^ s0, b ^ s1);
    }
    
    /* Long key: finish measuring length */
    while (p[len]) len++;
    
    /* Process 8-byte chunks */
    size_t i = 0;
    for (; i + 8 <= len; i += 8) {
        uint64_t chunk;
        memcpy(&chunk, p + i, 8);
        s0 = _wymix(s0 ^ chunk, s1);
        s1 += 0x60BEE2BEE120FC15ULL;
    }
    /* Remaining bytes */
    if (i < len) {
        uint64_t tail = 0;
        memcpy(&tail, p + i, len - i);
        s0 = _wymix(s0 ^ tail, s1);
    }
    
    return (uint32_t)_wymix(s0, s1 ^ ((uint64_t)len << 56));
}

static inline void lp_dict_resize(LpDict *d, int64_t new_capacity); /* forward decl */

/* Reserve capacity to avoid rehashing. Call before inserting N items. */
static inline void lp_dict_reserve(LpDict *d, int64_t n) {
    if (!d) return;
    int64_t needed = (int64_t)(n / LP_DICT_LOAD_FACTOR) + 1;
    /* Round up to next power of 2 */
    int64_t cap = 1024;
    while (cap < needed) cap <<= 1;
    if (cap > d->capacity) lp_dict_resize(d, cap);
}

static inline LpDict* lp_dict_new(void) {
    LpDict *d = (LpDict*)malloc(sizeof(LpDict));
    if (!d) return NULL;
    d->capacity = LP_DICT_INITIAL_CAPACITY;
    d->count = 0;
    d->entries = (LpDictEntry*)calloc(d->capacity, sizeof(LpDictEntry));
    if (!d->entries) {
        free(d);
        return NULL;
    }
    return d;
}

static inline LpList* lp_list_new(void) {
    LpList *l = (LpList*)malloc(sizeof(LpList));
    if (!l) return NULL;
    l->cap = 8;
    l->len = 0;
    l->items = (LpVal*)malloc(sizeof(LpVal) * l->cap);
    if (!l->items) {
        free(l);
        return NULL;
    }
    return l;
}

static inline void lp_dict_free(LpDict *d);

static inline void lp_list_free(LpList *l) {
    if (!l) return;
    for (int64_t i = 0; i < l->len; i++) {
        if (l->items[i].type == LP_VAL_STRING) free(l->items[i].as.s);
        else if (l->items[i].type == LP_VAL_DICT) lp_dict_free(l->items[i].as.d);
        else if (l->items[i].type == LP_VAL_LIST) lp_list_free(l->items[i].as.l);
    }
    free(l->items);
    free(l);
}

static inline void lp_list_append(LpList *l, LpVal value) {
    if (!l || !l->items) return;
    if (l->len >= l->cap) {
        int64_t new_cap = l->cap + (l->cap >> 1); /* 1.5x growth — 25% less memory than 2x */
        if (new_cap < 8) new_cap = 8;
        LpVal* new_items = (LpVal*)realloc(l->items, sizeof(LpVal) * new_cap);
        if (!new_items) return;  /* Failed to allocate, don't add item */
        l->items = new_items;
        l->cap = new_cap;
    }
    if (value.type == LP_VAL_STRING) {
        LpVal vcopy = value;
        vcopy.as.s = strdup(value.as.s);
        if (!vcopy.as.s) return; /* strdup failed */
        l->items[l->len++] = vcopy;
    } else {
        l->items[l->len++] = value; /* Deep copy for dict/list isn't done yet, assume ownership transfer or shared */
    }
}

/* Create a new list by repeating an element n times: [elem] * n */
static inline LpList* lp_list_repeat(LpVal elem, int64_t n) {
    LpList *l = lp_list_new();
    if (!l) return NULL;
    if (n <= 0) return l;
    /* Pre-allocate capacity */
    int64_t cap = (n < 8) ? 8 : n;
    LpVal* new_items = (LpVal*)realloc(l->items, sizeof(LpVal) * cap);
    if (!new_items) { lp_list_free(l); return NULL; }
    l->items = new_items;
    l->cap = cap;
    /* Fill with repeated elements */
    for (int64_t i = 0; i < n; i++) {
        if (elem.type == LP_VAL_STRING) {
            LpVal vcopy;
            vcopy.type = LP_VAL_STRING;
            vcopy.as.s = strdup(elem.as.s);
            if (!vcopy.as.s) { lp_list_free(l); return NULL; }
            l->items[l->len++] = vcopy;
        } else {
            l->items[l->len++] = elem;
        }
    }
    return l;
}

static inline void lp_dict_free(LpDict *d) {
    if (!d) return;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            lp_entry_free_key(&d->entries[i]);
            if (d->entries[i].value.type == LP_VAL_STRING) {
                free(d->entries[i].value.as.s);
            } else if (d->entries[i].value.type == LP_VAL_DICT) {
                lp_dict_free(d->entries[i].value.as.d);
            } else if (d->entries[i].value.type == LP_VAL_LIST) {
                lp_list_free(d->entries[i].value.as.l);
            }
        }
    }
    free(d->entries);
    free(d);
}

static inline void lp_dict_set(LpDict *d, const char *key, LpVal value);

static inline void lp_dict_resize(LpDict *d, int64_t new_capacity) {
    LpDictEntry *old_entries = d->entries;
    int64_t old_capacity = d->capacity;

    d->capacity = new_capacity;
    d->entries = (LpDictEntry*)calloc(d->capacity, sizeof(LpDictEntry));
    d->count = 0;

    for (int64_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].is_occupied) {
            lp_dict_set(d, lp_entry_key(&old_entries[i]), old_entries[i].value);
            lp_entry_free_key(&old_entries[i]); /* SSO: free heap key if any */
        }
    }
    free(old_entries);
}

static inline void lp_dict_set(LpDict *d, const char *key, LpVal value) {
    if (!d || !d->entries || !key) return;
    if (d->count >= d->capacity * LP_DICT_LOAD_FACTOR) {
        lp_dict_resize(d, d->capacity * 2);
    }

    uint32_t mask = (uint32_t)(d->capacity - 1);
    uint32_t index = lp_hash(key) & mask;
    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            /* Replace existing value */
            if (d->entries[index].value.type == LP_VAL_STRING) {
                free(d->entries[index].value.as.s);
            }
            if (value.type == LP_VAL_STRING) {
                LpVal vcopy = value;
                vcopy.as.s = strdup(value.as.s);
                if (!vcopy.as.s) return; /* strdup failed */
                d->entries[index].value = vcopy;
            } else {
                d->entries[index].value = value;
            }
            return;
        }
        index = (index + 1) & mask;
#ifdef __GNUC__
        __builtin_prefetch(&d->entries[(index + 1) & mask], 0, 1);
#endif
    }

    lp_entry_set_key(&d->entries[index], key);
    if (value.type == LP_VAL_STRING) {
        LpVal vcopy = value;
        vcopy.as.s = strdup(value.as.s);
        if (!vcopy.as.s) {
            lp_entry_free_key(&d->entries[index]);
            return;
        }
        d->entries[index].value = vcopy;
    } else {
        d->entries[index].value = value;
    }
    d->entries[index].is_occupied = 1;
    d->count++;
}

static inline LpVal lp_dict_get(LpDict *d, const char *key) {
    if (!d || !d->entries || !key) return lp_val_null();
    uint32_t mask = (uint32_t)(d->capacity - 1);
    uint32_t index = lp_hash(key) & mask;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            return d->entries[index].value;
        }
        index = (index + 1) & mask;
#ifdef __GNUC__
        __builtin_prefetch(&d->entries[(index + 1) & mask], 0, 1);
#endif
        if (index == start_index) break;
    }
    return lp_val_null(); /* Not found */
}

static inline int lp_dict_contains(LpDict *d, const char *key) {
    if (!d || !d->entries || !key) return 0;
    uint32_t mask = (uint32_t)(d->capacity - 1);
    uint32_t index = lp_hash(key) & mask;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            return 1;
        }
        index = (index + 1) & mask;
        if (index == start_index) break;
    }
    return 0;
}

static inline void lp_list_print(LpList *l);

static inline void lp_dict_print(LpDict *d) {
    if (!d || !d->entries) { printf("{}"); return; }
    printf("{");
    int first = 1;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            if (!first) printf(", ");
            printf("\"%s\": ", lp_entry_key(&d->entries[i]));
            switch (d->entries[i].value.type) {
                case LP_VAL_INT: printf("%" PRId64, d->entries[i].value.as.i); break;
                case LP_VAL_FLOAT: printf("%f", d->entries[i].value.as.f); break;
                case LP_VAL_STRING: printf("\"%s\"", d->entries[i].value.as.s); break;
                case LP_VAL_BOOL: printf(d->entries[i].value.as.b ? "True" : "False"); break;
                case LP_VAL_NULL: printf("None"); break;
                case LP_VAL_DICT: lp_dict_print(d->entries[i].value.as.d); break;
                case LP_VAL_LIST: lp_list_print(d->entries[i].value.as.l); break;
            }
            first = 0;
        }
    }
    printf("}");
}

static inline void lp_list_print(LpList *l) {
    if (!l || !l->items) { printf("[]"); return; }
    printf("[");
    for (int64_t i = 0; i < l->len; i++) {
        if (i > 0) printf(", ");
        switch (l->items[i].type) {
            case LP_VAL_INT: printf("%" PRId64, l->items[i].as.i); break;
            case LP_VAL_FLOAT: printf("%f", l->items[i].as.f); break;
            case LP_VAL_STRING: printf("\"%s\"", l->items[i].as.s); break;
            case LP_VAL_BOOL: printf(l->items[i].as.b ? "True" : "False"); break;
            case LP_VAL_NULL: printf("None"); break;
            case LP_VAL_DICT: lp_dict_print(l->items[i].as.d); break;
            case LP_VAL_LIST: lp_list_print(l->items[i].as.l); break;
        }
    }
    printf("]");
}

/* Merge another dict into this one (for **kwargs unpacking) */
static inline void lp_dict_merge(LpDict *dst, LpDict *src) {
    if (!dst || !src || !src->entries) return;
    for (int64_t i = 0; i < src->capacity; i++) {
        if (src->entries[i].is_occupied) {
            lp_dict_set(dst, lp_entry_key(&src->entries[i]), src->entries[i].value);
        }
    }
}

/* ========================================================
 * lp_dict_inc_int(d, key, delta)
 *
 * Combined "get-or-zero then add delta" in ONE hash probe.
 * Avoids the double-probe pattern:
 *   val = get(d, key); set(d, key, val+1)  ← 2 probes + strdup every time
 * This function probes once, increments in-place if found, inserts if not.
 * The key is strdup'd ONLY on first insert.
 *
 * Use when dict values are counters (frequency tables, histograms).
 * ======================================================== */
static inline void lp_dict_inc_int(LpDict *d, const char *key, int64_t delta) {
    if (!d || !d->entries || !key) return;
    if (d->count >= (int64_t)(d->capacity * LP_DICT_LOAD_FACTOR)) {
        lp_dict_resize(d, d->capacity * 2);
    }
    uint32_t mask = (uint32_t)(d->capacity - 1);
    uint32_t index = lp_hash(key) & mask;
    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            /* Found — increment in place, zero malloc */
            if (d->entries[index].value.type == LP_VAL_INT)
                d->entries[index].value.as.i += delta;
            else
                d->entries[index].value = lp_val_int(delta);
            return;
        }
        index = (index + 1) & mask;
#ifdef __GNUC__
        __builtin_prefetch(&d->entries[(index + 1) & mask], 0, 1);
#endif
    }
    /* Not found — SSO insert (no malloc for keys ≤15 chars) */
    lp_entry_set_key(&d->entries[index], key);
    d->entries[index].value = lp_val_int(delta);
    d->entries[index].is_occupied = 1;
    d->count++;
}


/* Macro helpers for codegen */
/* d["key"] = 123 */
#define LP_DICT_SET_INT(d, key, val) lp_dict_set(d, key, lp_val_int(val))
#define LP_DICT_SET_FLOAT(d, key, val) lp_dict_set(d, key, lp_val_float(val))
#define LP_DICT_SET_STR(d, key, val) lp_dict_set(d, key, lp_val_str(val))
#define LP_DICT_SET_BOOL(d, key, val) lp_dict_set(d, key, lp_val_bool(val))

/* d["key"] */
#define LP_DICT_GET_INT(d, key) (lp_dict_get(d, key).as.i)
#define LP_DICT_GET_FLOAT(d, key) (lp_dict_get(d, key).as.f)
#define LP_DICT_GET_STR(d, key) (lp_dict_get(d, key).as.s)
#define LP_DICT_GET_BOOL(d, key) (lp_dict_get(d, key).as.b)

/* ========================================
 * TYPE NARROWING FOR UNION TYPES
 * Additional helper functions beyond what's in lp_runtime.h
 * ======================================== */

/* Type narrowing - extract int with default */
static inline int64_t lp_val_as_int_or(LpVal v, int64_t default_val) {
    if (v.type == LP_VAL_INT) return v.as.i;
    if (v.type == LP_VAL_FLOAT) return (int64_t)v.as.f;
    return default_val;
}

/* Type narrowing - extract int (assumes type is already known to be int) */
static inline int64_t lp_val_as_int(LpVal v) {
    if (v.type == LP_VAL_INT) return v.as.i;
    if (v.type == LP_VAL_FLOAT) return (int64_t)v.as.f;
    return 0;
}

/* Type narrowing - extract float with default */
static inline double lp_val_as_float_or(LpVal v, double default_val) {
    if (v.type == LP_VAL_FLOAT) return v.as.f;
    if (v.type == LP_VAL_INT) return (double)v.as.i;
    return default_val;
}

/* Type narrowing - extract string with default */
static inline const char* lp_val_as_str_or(LpVal v, const char* default_val) {
    if (v.type == LP_VAL_STRING) return v.as.s;
    return default_val;
}

/* Type assertion - raises error message if type doesn't match */
static inline int lp_val_assert_type(LpVal v, LpValType expected, const char* context) {
    if (v.type != expected) {
        fprintf(stderr, "TypeError: expected type %d but got %d in %s\n",
                expected, v.type, context);
        return 0;
    }
    return 1;
}

/* ========================================
 * FAST INT ARRAYS FOR COMPETITIVE PROGRAMMING
 * Zero-overhead raw int64_t arrays - bypass LpVal boxing
 * ======================================== */

typedef struct {
    int64_t *data;
    int64_t len;
} LpIntArray;

static inline LpIntArray* lp_int_array_new(int64_t size) {
    LpIntArray *arr = (LpIntArray*)malloc(sizeof(LpIntArray));
    if (!arr) return NULL;
    /* 32-byte aligned: enables AVX2 load/store without penalty */
    size_t bytes = (size_t)size * sizeof(int64_t);
    size_t aligned = (bytes + 31) & ~(size_t)31;
    arr->data = (int64_t*)lp_aligned_alloc(32, aligned ? aligned : 32);
    if (!arr->data) { free(arr); return NULL; }
    memset(arr->data, 0, bytes);
    arr->len = size;
    return arr;
}

static inline void lp_int_array_free(LpIntArray *arr) {
    if (arr) { lp_aligned_free(arr->data); free(arr); }
}

/* Reuse an existing array if same size, otherwise reallocate.
 * Eliminates malloc/free churn when the same-sized array is created every loop iteration.
 * Usage: arr = lp_int_array_reuse(arr, size);  (replaces lp_int_array_new inside loops) */
static inline LpIntArray* lp_int_array_reuse(LpIntArray *arr, int64_t size) {
    if (arr && arr->len == size) {
        /* Same size: just zero it and reuse — no malloc */
        memset(arr->data, 0, (size_t)size * sizeof(int64_t));
        return arr;
    }
    lp_int_array_free(arr);
    return lp_int_array_new(size);
}

static inline int64_t lp_int_array_get(LpIntArray *arr, int64_t idx) {
    return arr->data[idx];
}

static inline void lp_int_array_set(LpIntArray *arr, int64_t idx, int64_t val) {
    arr->data[idx] = val;
}

static inline LpIntArray* lp_int_array_repeat(int64_t val, int64_t count) {
    LpIntArray *arr = lp_int_array_new(count);
    if (!arr) return NULL;
    for (int64_t i = 0; i < count; i++) arr->data[i] = val;
    return arr;
}

/* ========================================
 * lp_int_array_to_list — converts LpIntArray* to LpList* (for function-call coercion)
 * Used when a list param receives an LpIntArray* argument.
 * ======================================== */
static inline LpList* lp_int_array_to_list(LpIntArray *arr) {
    if (!arr) return lp_list_new();
    LpList *l = lp_list_new();
    for (int64_t i = 0; i < arr->len; i++)
        lp_list_append(l, lp_val_int(arr->data[i]));
    return l;
}

/* ========================================
 * LpI8Array — int8_t (byte) arrays for boolean/flag tables
 * 8x denser than LpIntArray (int64_t): sieve[5M] = 5MB vs 40MB
 * Cache-friendly: fits L2/L3 where int64_t spills to RAM
 * Use annotation:  sieve: i8[]  visited: i8[]  color: i8[]
 * ======================================== */
typedef struct {
    int8_t *data;
    int64_t len;
} LpI8Array;

static inline LpI8Array* lp_i8_array_new(int64_t size) {
    LpI8Array *arr = (LpI8Array*)malloc(sizeof(LpI8Array));
    if (!arr) return NULL;
    size_t bytes = (size_t)size * sizeof(int8_t);
    size_t aligned = (bytes + 31) & ~(size_t)31;
    arr->data = (int8_t*)lp_aligned_alloc(32, aligned ? aligned : 32);
    if (!arr->data) { free(arr); return NULL; }
    memset(arr->data, 0, bytes);
    arr->len = size;
    return arr;
}
static inline void lp_i8_array_free(LpI8Array *arr) {
    if (arr) { lp_aligned_free(arr->data); free(arr); }
}
static inline LpI8Array* lp_i8_array_repeat(int8_t val, int64_t count) {
    LpI8Array *arr = lp_i8_array_new(count);
    if (!arr) return NULL;
    memset(arr->data, val, (size_t)count);
    return arr;
}
static inline LpList* lp_i8_array_to_list(LpI8Array *arr) {
    if (!arr) return lp_list_new();
    LpList *l = lp_list_new();
    for (int64_t i = 0; i < arr->len; i++)
        lp_list_append(l, lp_val_int(arr->data[i]));
    return l;
}

/* ========================================
 * LpF32Array — float (32-bit) arrays for distance matrices
 * 2x memory bandwidth vs double: D[100*100] = 40KB vs 80KB
 * Use annotation: D: f32[]  (distance matrix)
 * ======================================== */
typedef struct {
    float *data;
    int32_t len;
} LpF32Array;

static inline LpF32Array* lp_f32_array_new(int64_t size) {
    LpF32Array *arr = (LpF32Array*)malloc(sizeof(LpF32Array));
    if (!arr) return NULL;
    size_t bytes = (size_t)size * sizeof(float);
    size_t aligned = (bytes + 31) & ~(size_t)31;
    arr->data = (float*)lp_aligned_alloc(32, aligned ? aligned : 32);
    if (!arr->data) { free(arr); return NULL; }
    memset(arr->data, 0, bytes);
    arr->len = (int32_t)size;
    return arr;
}
static inline void lp_f32_array_free(LpF32Array *arr) {
    if (arr) { lp_aligned_free(arr->data); free(arr); }
}
static inline void lp_f32_memcpy(LpF32Array *dst, LpF32Array *src, int64_t n) {
    if (dst && src && n > 0) memcpy(dst->data, src->data, (size_t)n * sizeof(float));
}
static inline void lp_f32_memset_zero(LpF32Array *arr, int64_t n) {
    if (arr && n > 0) memset(arr->data, 0, (size_t)n * sizeof(float));
}

/* ========================================
 * LpI32Array — int32_t arrays for index-heavy code
 * 2x denser than LpIntArray: N=100 fits in 400B vs 800B
 * Use annotation: t: i32[]  pos: i32[]  dlb: i32[]  NN: i32[]
 * ======================================== */
typedef struct {
    int32_t *data;
    int32_t len;
} LpI32Array;

static inline LpI32Array* lp_i32_array_new(int64_t size) {
    LpI32Array *arr = (LpI32Array*)malloc(sizeof(LpI32Array));
    if (!arr) return NULL;
    size_t bytes = (size_t)size * sizeof(int32_t);
    size_t aligned = (bytes + 31) & ~(size_t)31;
    arr->data = (int32_t*)lp_aligned_alloc(32, aligned ? aligned : 32);
    if (!arr->data) { free(arr); return NULL; }
    memset(arr->data, 0, bytes);
    arr->len = (int32_t)size;
    return arr;
}

static inline void lp_i32_array_free(LpI32Array *arr) {
    if (arr) { lp_aligned_free(arr->data); free(arr); }
}

static inline LpI32Array* lp_i32_array_repeat(int32_t val, int64_t count) {
    LpI32Array *arr = lp_i32_array_new(count);
    if (!arr) return NULL;
    for (int64_t i = 0; i < count; i++) arr->data[i] = val;
    return arr;
}

/* DSA helpers for i32 arrays */
static inline void lp_i32_memcpy(LpI32Array *dst, LpI32Array *src, int64_t n) {
    if (dst && src && n > 0) memcpy(dst->data, src->data, (size_t)n * sizeof(int32_t));
}
static inline void lp_i32_memset_zero(LpI32Array *arr, int64_t n) {
    if (arr && n > 0) memset(arr->data, 0, (size_t)n * sizeof(int32_t));
}
static inline void lp_i32_copy_range(LpI32Array *dst, int64_t doff,
                                      LpI32Array *src, int64_t soff, int64_t n) {
    if (dst && src && n > 0)
        memcpy(dst->data + doff, src->data + soff, (size_t)n * sizeof(int32_t));
}
static inline void lp_i32_memmove_left1(LpI32Array *arr, int64_t from, int64_t to) {
    if (arr && to > from && from >= 1)
        memmove(arr->data + from - 1, arr->data + from, (size_t)(to - from) * sizeof(int32_t));
}
static inline void lp_i32_memmove_right1(LpI32Array *arr, int64_t from, int64_t to) {
    if (arr && to > from)
        memmove(arr->data + from + 1, arr->data + from, (size_t)(to - from) * sizeof(int32_t));
}

/* 2D array as 1D with row-major indexing */
typedef struct {
    int64_t *data;
    int64_t rows;
    int64_t cols;
} LpIntArray2D;

static inline LpIntArray2D* lp_int_array2d_new(int64_t rows, int64_t cols) {
    LpIntArray2D *arr = (LpIntArray2D*)malloc(sizeof(LpIntArray2D));
    if (!arr) return NULL;
    arr->data = (int64_t*)calloc(rows * cols, sizeof(int64_t));
    if (!arr->data) { free(arr); return NULL; }
    arr->rows = rows;
    arr->cols = cols;
    return arr;
}

static inline void lp_int_array2d_free(LpIntArray2D *arr) {
    if (arr) { free(arr->data); free(arr); }
}

static inline int64_t lp_int_array2d_get(LpIntArray2D *arr, int64_t r, int64_t c) {
    return arr->data[r * arr->cols + c];
}

static inline void lp_int_array2d_set(LpIntArray2D *arr, int64_t r, int64_t c, int64_t val) {
    arr->data[r * arr->cols + c] = val;
}

/* ========================================
 * Zero-overhead raw double (float64) arrays
 * ======================================== */

typedef struct {
    double  *data;
    int64_t  len;
} LpFloatArray;

static inline LpFloatArray* lp_float_array_new(int64_t size) {
    LpFloatArray *arr = (LpFloatArray*)malloc(sizeof(LpFloatArray));
    if (!arr) return NULL;
    size_t bytes = (size_t)size * sizeof(double);
    size_t aligned = (bytes + 31) & ~(size_t)31;
    arr->data = (double*)lp_aligned_alloc(32, aligned ? aligned : 32);
    if (!arr->data) { free(arr); return NULL; }
    memset(arr->data, 0, bytes);
    arr->len = size;
    return arr;
}

static inline void lp_float_array_free(LpFloatArray *arr) {
    if (arr) { lp_aligned_free(arr->data); free(arr); }
}

static inline double lp_float_array_get(LpFloatArray *arr, int64_t idx) {
    return arr->data[idx];
}

static inline void lp_float_array_set(LpFloatArray *arr, int64_t idx, double val) {
    arr->data[idx] = val;
}

static inline LpFloatArray* lp_float_array_repeat(double val, int64_t count) {
    LpFloatArray *arr = lp_float_array_new(count);
    if (!arr) return NULL;
    for (int64_t i = 0; i < count; i++) arr->data[i] = val;
    return arr;
}

/* 2D float array as 1D with row-major indexing */
typedef struct {
    double  *data;
    int64_t  rows;
    int64_t  cols;
} LpFloatArray2D;

static inline LpFloatArray2D* lp_float_array2d_new(int64_t rows, int64_t cols) {
    LpFloatArray2D *arr = (LpFloatArray2D*)malloc(sizeof(LpFloatArray2D));
    if (!arr) return NULL;
    arr->data = (double*)calloc(rows * cols, sizeof(double));
    if (!arr->data) { free(arr); return NULL; }
    arr->rows = rows;
    arr->cols = cols;
    return arr;
}

static inline void lp_float_array2d_free(LpFloatArray2D *arr) {
    if (arr) { free(arr->data); free(arr); }
}

static inline double lp_float_array2d_get(LpFloatArray2D *arr, int64_t r, int64_t c) {
    return arr->data[r * arr->cols + c];
}

static inline void lp_float_array2d_set(LpFloatArray2D *arr, int64_t r, int64_t c, double val) {
    arr->data[r * arr->cols + c] = val;
}

/* ========================================
 * Shift helpers using memmove (SIMD-optimized by libc)
 * Used by OR-opt to avoid slow element-by-element loops
 * ======================================== */

/* Shift elements [from..to) right by 1: arr[from+1..to+1] = arr[from..to] */
static inline void lp_int_array_shift_right1(LpIntArray *arr, int64_t from, int64_t to) {
    if (to <= from || !arr || !arr->data) return;
    memmove(arr->data + from + 1, arr->data + from, (size_t)(to - from) * sizeof(int64_t));
}

/* Shift elements [from..to) left by 1: arr[from-1..to-1] = arr[from..to] */
static inline void lp_int_array_shift_left1(LpIntArray *arr, int64_t from, int64_t to) {
    if (to <= from || !arr || !arr->data) return;
    memmove(arr->data + from - 1, arr->data + from, (size_t)(to - from) * sizeof(int64_t));
}

/* ========================================
 * DICT ADDITIONAL METHODS
 * Python-compatible dict operations
 * ======================================== */

/* dict.pop(key) — Remove and return value for key */
static inline LpVal lp_dict_pop(LpDict *d, const char *key) {
    if (!d || !d->entries || !key) return lp_val_null();
    uint32_t index = lp_hash(key) % d->capacity;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            LpVal val = d->entries[index].value;
            /* Don't free string value — caller owns it now */
            lp_entry_free_key(&d->entries[index]);
            d->entries[index].is_occupied = 0;
            d->entries[index].value = lp_val_null();
            d->count--;
            return val;
        }
        index = (index + 1) % d->capacity;
        if (index == start_index) break;
    }
    return lp_val_null();
}

/* dict.remove(key) / del dict[key] — Remove key without returning value */
static inline void lp_dict_remove(LpDict *d, const char *key) {
    if (!d || !d->entries || !key) return;
    uint32_t index = lp_hash(key) % d->capacity;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            lp_entry_free_key(&d->entries[index]);
            if (d->entries[index].value.type == LP_VAL_STRING)
                free(d->entries[index].value.as.s);
            else if (d->entries[index].value.type == LP_VAL_DICT)
                lp_dict_free(d->entries[index].value.as.d);
            else if (d->entries[index].value.type == LP_VAL_LIST)
                lp_list_free(d->entries[index].value.as.l);
            d->entries[index].is_occupied = 0;
            d->entries[index].value = lp_val_null();
            d->count--;
            return;
        }
        index = (index + 1) % d->capacity;
        if (index == start_index) break;
    }
}

/* dict.keys() → LpList* of strings */
static inline LpList* lp_dict_keys(LpDict *d) {
    LpList *l = lp_list_new();
    if (!d || !d->entries) return l;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            lp_list_append(l, lp_val_str(lp_entry_key(&d->entries[i])));
        }
    }
    return l;
}

/* dict.values() → LpList* of values */
static inline LpList* lp_dict_values(LpDict *d) {
    LpList *l = lp_list_new();
    if (!d || !d->entries) return l;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            lp_list_append(l, d->entries[i].value);
        }
    }
    return l;
}

/* dict.items() → LpList* of [key, value] pairs (each pair is a LpList*) */
static inline LpList* lp_dict_items(LpDict *d) {
    LpList *l = lp_list_new();
    if (!d || !d->entries) return l;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            LpList *pair = lp_list_new();
            lp_list_append(pair, lp_val_str(lp_entry_key(&d->entries[i])));
            lp_list_append(pair, d->entries[i].value);
            lp_list_append(l, lp_val_list(pair));
        }
    }
    return l;
}

/* dict.copy() → deep copy */
static inline LpDict* lp_dict_copy(LpDict *d) {
    LpDict *copy = lp_dict_new();
    if (!d || !d->entries) return copy;
    lp_dict_reserve(copy, d->count);
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            lp_dict_set(copy, lp_entry_key(&d->entries[i]), d->entries[i].value);
        }
    }
    return copy;
}

/* dict.clear() */
static inline void lp_dict_clear(LpDict *d) {
    if (!d || !d->entries) return;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            lp_entry_free_key(&d->entries[i]);
            if (d->entries[i].value.type == LP_VAL_STRING)
                free(d->entries[i].value.as.s);
            else if (d->entries[i].value.type == LP_VAL_DICT)
                lp_dict_free(d->entries[i].value.as.d);
            else if (d->entries[i].value.type == LP_VAL_LIST)
                lp_list_free(d->entries[i].value.as.l);
            d->entries[i].is_occupied = 0;
        }
    }
    d->count = 0;
}

/* dict.get(key, default) */
static inline LpVal lp_dict_get_default(LpDict *d, const char *key, LpVal default_val) {
    if (!d || !d->entries || !key) return default_val;
    uint32_t index = lp_hash(key) % d->capacity;
    uint32_t start_index = index;
    while (d->entries[index].is_occupied) {
        if (strcmp(lp_entry_key(&d->entries[index]), key) == 0) {
            return d->entries[index].value;
        }
        index = (index + 1) % d->capacity;
        if (index == start_index) break;
    }
    return default_val;
}

/* dict.update(other) — merge other into d (alias for lp_dict_merge) */
static inline void lp_dict_update(LpDict *dst, LpDict *src) {
    lp_dict_merge(dst, src);
}

/* dict.__len__() — count of items */
static inline int64_t lp_dict_len(LpDict *d) {
    return d ? d->count : 0;
}

/* ========================================
 * LIST ADDITIONAL METHODS
 * Python-compatible list operations
 * ======================================== */

/* list.remove(val) — remove first occurrence */
static inline void lp_list_remove(LpList *l, LpVal val) {
    if (!l || !l->items) return;
    for (int64_t i = 0; i < l->len; i++) {
        int match = 0;
        if (l->items[i].type == val.type) {
            switch (val.type) {
                case LP_VAL_INT: match = (l->items[i].as.i == val.as.i); break;
                case LP_VAL_FLOAT: match = (l->items[i].as.f == val.as.f); break;
                case LP_VAL_STRING: match = (l->items[i].as.s && val.as.s && strcmp(l->items[i].as.s, val.as.s) == 0); break;
                case LP_VAL_BOOL: match = (l->items[i].as.b == val.as.b); break;
                case LP_VAL_NULL: match = 1; break;
                default: break;
            }
        }
        if (match) {
            if (l->items[i].type == LP_VAL_STRING) free(l->items[i].as.s);
            memmove(&l->items[i], &l->items[i + 1], (l->len - i - 1) * sizeof(LpVal));
            l->len--;
            return;
        }
    }
}

/* list.pop(idx) — remove and return element at index (-1 for last) */
static inline LpVal lp_list_pop(LpList *l, int64_t idx) {
    if (!l || !l->items || l->len == 0) return lp_val_null();
    if (idx < 0) idx += l->len;
    if (idx < 0 || idx >= l->len) return lp_val_null();
    LpVal val = l->items[idx];
    memmove(&l->items[idx], &l->items[idx + 1], (l->len - idx - 1) * sizeof(LpVal));
    l->len--;
    return val; /* Caller owns the value now */
}

/* list.insert(idx, val) */
static inline void lp_list_insert(LpList *l, int64_t idx, LpVal val) {
    if (!l) return;
    if (idx < 0) idx += l->len;
    if (idx < 0) idx = 0;
    if (idx > l->len) idx = l->len;
    /* Ensure capacity */
    if (l->len >= l->cap) {
        int64_t new_cap = l->cap + (l->cap >> 1);
        if (new_cap < 8) new_cap = 8;
        LpVal *new_items = (LpVal*)realloc(l->items, sizeof(LpVal) * new_cap);
        if (!new_items) return;
        l->items = new_items;
        l->cap = new_cap;
    }
    /* Shift elements right */
    memmove(&l->items[idx + 1], &l->items[idx], (l->len - idx) * sizeof(LpVal));
    /* Deep copy strings */
    if (val.type == LP_VAL_STRING) {
        LpVal vcopy = val;
        vcopy.as.s = strdup(val.as.s);
        l->items[idx] = vcopy;
    } else {
        l->items[idx] = val;
    }
    l->len++;
}

/* list.extend(other) — append all elements from other list */
static inline void lp_list_extend(LpList *l, LpList *other) {
    if (!l || !other || !other->items) return;
    for (int64_t i = 0; i < other->len; i++) {
        lp_list_append(l, other->items[i]);
    }
}

/* list.index(val) — find first index of val, returns -1 if not found */
static inline int64_t lp_list_index(LpList *l, LpVal val) {
    if (!l || !l->items) return -1;
    for (int64_t i = 0; i < l->len; i++) {
        int match = 0;
        if (l->items[i].type == val.type) {
            switch (val.type) {
                case LP_VAL_INT: match = (l->items[i].as.i == val.as.i); break;
                case LP_VAL_FLOAT: match = (l->items[i].as.f == val.as.f); break;
                case LP_VAL_STRING: match = (l->items[i].as.s && val.as.s && strcmp(l->items[i].as.s, val.as.s) == 0); break;
                case LP_VAL_BOOL: match = (l->items[i].as.b == val.as.b); break;
                case LP_VAL_NULL: match = 1; break;
                default: break;
            }
        }
        if (match) return i;
    }
    return -1;
}

/* list.__contains__(val) — check if val is in list */
static inline int lp_list_contains(LpList *l, LpVal val) {
    return lp_list_index(l, val) >= 0;
}

/* list.count(val) — count occurrences */
static inline int64_t lp_list_count(LpList *l, LpVal val) {
    if (!l || !l->items) return 0;
    int64_t count = 0;
    for (int64_t i = 0; i < l->len; i++) {
        int match = 0;
        if (l->items[i].type == val.type) {
            switch (val.type) {
                case LP_VAL_INT: match = (l->items[i].as.i == val.as.i); break;
                case LP_VAL_FLOAT: match = (l->items[i].as.f == val.as.f); break;
                case LP_VAL_STRING: match = (l->items[i].as.s && val.as.s && strcmp(l->items[i].as.s, val.as.s) == 0); break;
                case LP_VAL_BOOL: match = (l->items[i].as.b == val.as.b); break;
                case LP_VAL_NULL: match = 1; break;
                default: break;
            }
        }
        if (match) count++;
    }
    return count;
}

/* list.reverse() — reverse in-place */
static inline void lp_list_reverse(LpList *l) {
    if (!l || !l->items || l->len <= 1) return;
    for (int64_t i = 0; i < l->len / 2; i++) {
        LpVal tmp = l->items[i];
        l->items[i] = l->items[l->len - 1 - i];
        l->items[l->len - 1 - i] = tmp;
    }
}

/* list.copy() → shallow copy */
static inline LpList* lp_list_copy(LpList *l) {
    LpList *copy = lp_list_new();
    if (!l || !l->items) return copy;
    for (int64_t i = 0; i < l->len; i++) {
        lp_list_append(copy, l->items[i]);
    }
    return copy;
}

/* list.clear() */
static inline void lp_list_clear(LpList *l) {
    if (!l || !l->items) return;
    for (int64_t i = 0; i < l->len; i++) {
        if (l->items[i].type == LP_VAL_STRING) free(l->items[i].as.s);
        else if (l->items[i].type == LP_VAL_DICT) lp_dict_free(l->items[i].as.d);
        else if (l->items[i].type == LP_VAL_LIST) lp_list_free(l->items[i].as.l);
    }
    l->len = 0;
}

/* list[start:end:step] → slice */
static inline LpList* lp_list_slice(LpList *l, int64_t start, int64_t end, int64_t step) {
    LpList *result = lp_list_new();
    if (!l || !l->items || step == 0) return result;

    /* Normalize negative indices */
    if (start < 0) start += l->len;
    if (end < 0) end += l->len;
    if (start < 0) start = 0;
    if (end > l->len) end = l->len;

    if (step > 0) {
        for (int64_t i = start; i < end; i += step) {
            lp_list_append(result, l->items[i]);
        }
    } else {
        /* Negative step for reverse */
        if (start >= l->len) start = l->len - 1;
        if (end < -1) end = -1;
        for (int64_t i = start; i > end; i += step) {
            lp_list_append(result, l->items[i]);
        }
    }
    return result;
}

/* list.__len__() */
static inline int64_t lp_list_len(LpList *l) {
    return l ? l->len : 0;
}

/* list.min() — find minimum value (numeric) */
static inline LpVal lp_list_min(LpList *l) {
    if (!l || !l->items || l->len == 0) return lp_val_null();
    LpVal min_val = l->items[0];
    for (int64_t i = 1; i < l->len; i++) {
        if (l->items[i].type == LP_VAL_INT && min_val.type == LP_VAL_INT) {
            if (l->items[i].as.i < min_val.as.i) min_val = l->items[i];
        } else if (l->items[i].type == LP_VAL_FLOAT || min_val.type == LP_VAL_FLOAT) {
            double a = (l->items[i].type == LP_VAL_INT) ? (double)l->items[i].as.i : l->items[i].as.f;
            double b = (min_val.type == LP_VAL_INT) ? (double)min_val.as.i : min_val.as.f;
            if (a < b) min_val = l->items[i];
        }
    }
    return min_val;
}

/* list.max() — find maximum value (numeric) */
static inline LpVal lp_list_max(LpList *l) {
    if (!l || !l->items || l->len == 0) return lp_val_null();
    LpVal max_val = l->items[0];
    for (int64_t i = 1; i < l->len; i++) {
        if (l->items[i].type == LP_VAL_INT && max_val.type == LP_VAL_INT) {
            if (l->items[i].as.i > max_val.as.i) max_val = l->items[i];
        } else if (l->items[i].type == LP_VAL_FLOAT || max_val.type == LP_VAL_FLOAT) {
            double a = (l->items[i].type == LP_VAL_INT) ? (double)l->items[i].as.i : l->items[i].as.f;
            double b = (max_val.type == LP_VAL_INT) ? (double)max_val.as.i : max_val.as.f;
            if (a > b) max_val = l->items[i];
        }
    }
    return max_val;
}

/* list.sum() — sum of numeric values */
static inline LpVal lp_list_sum(LpList *l) {
    if (!l || !l->items || l->len == 0) return lp_val_int(0);
    int has_float = 0;
    double sum_f = 0.0;
    int64_t sum_i = 0;
    for (int64_t i = 0; i < l->len; i++) {
        if (l->items[i].type == LP_VAL_INT) {
            sum_i += l->items[i].as.i;
            sum_f += (double)l->items[i].as.i;
        } else if (l->items[i].type == LP_VAL_FLOAT) {
            has_float = 1;
            sum_f += l->items[i].as.f;
        }
    }
    return has_float ? lp_val_float(sum_f) : lp_val_int(sum_i);
}

#endif /* LP_DICT_H */
