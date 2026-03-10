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

#define LP_DICT_INITIAL_CAPACITY 16
#define LP_DICT_LOAD_FACTOR 0.75

/* Type wrapper for values in the dict */
typedef enum {
    LP_VAL_INT,
    LP_VAL_FLOAT,
    LP_VAL_STRING,
    LP_VAL_BOOL,
    LP_VAL_NULL
} LpValType;

typedef struct {
    LpValType type;
    union {
        int64_t i;
        double f;
        char *s;
        int b;
    } as;
} LpVal;

static inline LpVal lp_val_int(int64_t i) { LpVal v; v.type = LP_VAL_INT; v.as.i = i; return v; }
static inline LpVal lp_val_float(double f) { LpVal v; v.type = LP_VAL_FLOAT; v.as.f = f; return v; }
static inline LpVal lp_val_str(const char *s) {
    LpVal v; v.type = LP_VAL_STRING; v.as.s = strdup(s); return v;
}
static inline LpVal lp_val_bool(int b) { LpVal v; v.type = LP_VAL_BOOL; v.as.b = b; return v; }
static inline LpVal lp_val_null(void) { LpVal v; v.type = LP_VAL_NULL; v.as.i = 0; return v; }

/* Dict Entry */
typedef struct {
    char *key;
    LpVal value;
    int is_occupied;
} LpDictEntry;

/* The Dictionary */
typedef struct {
    LpDictEntry *entries;
    int64_t capacity;
    int64_t count;
} LpDict;

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
    d->capacity = LP_DICT_INITIAL_CAPACITY;
    d->count = 0;
    d->entries = (LpDictEntry*)calloc(d->capacity, sizeof(LpDictEntry));
    return d;
}

static inline void lp_dict_free(LpDict *d) {
    if (!d) return;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            free(d->entries[i].key);
            if (d->entries[i].value.type == LP_VAL_STRING) {
                free(d->entries[i].value.as.s);
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
                d->entries[index].value = vcopy;
            } else {
                d->entries[index].value = value;
            }
            return;
        }
        index = (index + 1) % d->capacity;
    }

    d->entries[index].key = strdup(key);
    if (value.type == LP_VAL_STRING) {
        LpVal vcopy = value;
        vcopy.as.s = strdup(value.as.s);
        d->entries[index].value = vcopy;
    } else {
        d->entries[index].value = value;
    }
    d->entries[index].is_occupied = 1;
    d->count++;
}

static inline LpVal lp_dict_get(LpDict *d, const char *key) {
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

static inline void lp_dict_print(LpDict *d) {
    printf("{");
    int first = 1;
    for (int64_t i = 0; i < d->capacity; i++) {
        if (d->entries[i].is_occupied) {
            if (!first) printf(", ");
            printf("\"%s\": ", d->entries[i].key);
            switch (d->entries[i].value.type) {
                case LP_VAL_INT: printf("%lld", (long long)d->entries[i].value.as.i); break;
                case LP_VAL_FLOAT: printf("%f", d->entries[i].value.as.f); break;
                case LP_VAL_STRING: printf("\"%s\"", d->entries[i].value.as.s); break;
                case LP_VAL_BOOL: printf(d->entries[i].value.as.b ? "True" : "False"); break;
                case LP_VAL_NULL: printf("None"); break;
            }
            first = 0;
        }
    }
    printf("}");
}

/* Merge another dict into this one (for **kwargs unpacking) */
static inline void lp_dict_merge(LpDict *dst, LpDict *src) {
    if (!src) return;
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

#endif /* LP_DICT_H */
