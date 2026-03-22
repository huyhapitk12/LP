/*
 * LP Native Dictionary — Zero-Overhead C Hash Map Implementation
 * Maps Python's dict to a native C open-addressing hash table.
 * Supports heterogeneous values (int, float, string, bool) via LpVal union.
 */

#ifndef LP_DICT_H
#define LP_DICT_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define LP_DICT_INITIAL_CAPACITY 16
#define LP_DICT_LOAD_FACTOR 0.75

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

/* Dict Entry */
typedef struct {
    char *key;
    LpVal value;
    int is_occupied;
} LpDictEntry;

/* The Dictionary */
struct LpDict {
    LpDictEntry *entries;
    int64_t capacity;
    int64_t count;
};

/* Hash function (FNV-1a) */
static inline uint32_t lp_hash(const char* key) {
    uint32_t hash = 2166136261u;
    for (const char* p = key; *p; p++) {
        hash ^= (uint32_t)(*p);
        hash *= 16777619;
    }
    return hash;
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
        int64_t new_cap = l->cap * 2;
        if (new_cap == 0) new_cap = 8;  /* Initial capacity */
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
            free(d->entries[i].key);
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
            lp_dict_set(d, old_entries[i].key, old_entries[i].value);
            free(old_entries[i].key); /* Free the old key string, lp_dict_set duplicates it */
        }
    }
    free(old_entries);
}

static inline void lp_dict_set(LpDict *d, const char *key, LpVal value) {
    if (!d || !d->entries || !key) return;
    if (d->count >= d->capacity * LP_DICT_LOAD_FACTOR) {
        lp_dict_resize(d, d->capacity * 2);
    }

    uint32_t index = lp_hash(key) % d->capacity;
    while (d->entries[index].is_occupied) {
        if (strcmp(d->entries[index].key, key) == 0) {
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
        index = (index + 1) % d->capacity;
    }

    char *key_copy = strdup(key);
    if (!key_copy) return; /* strdup failed */
    d->entries[index].key = key_copy;
    if (value.type == LP_VAL_STRING) {
        LpVal vcopy = value;
        vcopy.as.s = strdup(value.as.s);
        if (!vcopy.as.s) {
            free(key_copy);
            d->entries[index].key = NULL;
            return; /* strdup failed */
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
    uint32_t index = lp_hash(key) % d->capacity;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(d->entries[index].key, key) == 0) {
            return d->entries[index].value;
        }
        index = (index + 1) % d->capacity;
        if (index == start_index) break;
    }
    return lp_val_null(); /* Not found */
}

static inline int lp_dict_contains(LpDict *d, const char *key) {
    if (!d || !d->entries || !key) return 0;
    uint32_t index = lp_hash(key) % d->capacity;
    uint32_t start_index = index;

    while (d->entries[index].is_occupied) {
        if (strcmp(d->entries[index].key, key) == 0) {
            return 1;
        }
        index = (index + 1) % d->capacity;
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
            printf("\"%s\": ", d->entries[i].key);
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
            lp_dict_set(dst, src->entries[i].key, src->entries[i].value);
        }
    }
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
    arr->data = (int64_t*)calloc(size, sizeof(int64_t));
    if (!arr->data) { free(arr); return NULL; }
    arr->len = size;
    return arr;
}

static inline void lp_int_array_free(LpIntArray *arr) {
    if (arr) { free(arr->data); free(arr); }
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
    arr->data = (float*)calloc(size, sizeof(float));
    if (!arr->data) { free(arr); return NULL; }
    arr->len = (int32_t)size;
    return arr;
}
static inline void lp_f32_array_free(LpF32Array *arr) {
    if (arr) { free(arr->data); free(arr); }
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
    arr->data = (int32_t*)calloc(size, sizeof(int32_t));
    if (!arr->data) { free(arr); return NULL; }
    arr->len = (int32_t)size;
    return arr;
}

static inline void lp_i32_array_free(LpI32Array *arr) {
    if (arr) { free(arr->data); free(arr); }
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
    arr->data = (double*)calloc(size, sizeof(double));
    if (!arr->data) { free(arr); return NULL; }
    arr->len = size;
    return arr;
}

static inline void lp_float_array_free(LpFloatArray *arr) {
    if (arr) { free(arr->data); free(arr); }
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

#endif /* LP_DICT_H */
