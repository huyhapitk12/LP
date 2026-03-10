#ifndef LP_RUNTIME_H
#define LP_RUNTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "lp_dict.h"
#include "lp_set.h"
#include "lp_tuple.h"
#include "lp_io.h"
#include "lp_exception.h"
#include "lp_array.h"
#include "lp_args.h"

static inline void lp_args_extend_array(LpVarArgs *vargs, LpArray arr) {
    /* Special helper to expand an LpArray into LpVarArgs */
    for (int i = 0; i < arr.shape[0]; i++) {
        double *val_ptr = (double*)malloc(sizeof(double));
        *val_ptr = arr.data[i];
        lp_args_push(vargs, 1, val_ptr); /* 1 corresponds to LP_FLOAT generally but generic int */
    }
}

static inline LpArray lp_args_to_array(LpVarArgs *vargs) {
    LpArray arr;
    arr.data = (double*)malloc(vargs->count * sizeof(double));
    arr.len = vargs->count;
    arr.cap = vargs->count;
    arr.shape[0] = vargs->count;
    arr.shape[1] = 0;
    arr.shape[2] = 0;
    arr.shape[3] = 0;
    for (int i = 0; i < vargs->count; i++) {
        /* Since we don't have deep run-time types mapped cleanly, fallback to float casts or pointer casting */
        if (vargs->types[i] == 1) {
            arr.data[i] = *((double*)vargs->values[i]);
        } else {
            arr.data[i] = (double)(intptr_t)vargs->values[i];
        }
    }
    return arr;
}

/* Print helpers */
static inline void lp_print_int(int64_t v) { printf("%" PRId64 "\n", v); }
static inline void lp_print_float(double v) { printf("%g\n", v); }
static inline void lp_print_str(const char *v) { printf("%s\n", v); }
static inline void lp_print_bool(int v) { printf("%s\n", v ? "True" : "False"); }
static inline void lp_print_none(void) { printf("None\n"); }

static inline void lp_print_tuple(LpTuple *t) {
    printf("(");
    if (t) {
        for (int i = 0; i < t->count; i++) {
            if (i > 0) printf(", ");
            printf("%" PRId64, (int64_t)(intptr_t)t->items[i]);
        }
    }
    printf(")\n");
}

/* C11 Generic print for unknown/inferred types (like object properties) */
#define lp_print_generic(x) _Generic((x), \
    double: lp_print_float, \
    float: lp_print_float, \
    char *: lp_print_str, \
    const char *: lp_print_str, \
    LpTuple*: lp_print_tuple, \
    default: lp_print_int \
)(x)

static inline void lp_print_dict(LpDict *d) {
    if (d) lp_dict_print(d);
    else printf("None");
    printf("\n");
}

static inline void lp_print_set(LpSet *s) {
    if (s) lp_set_print(s);
    else printf("None");
    printf("\n");
}

#ifndef LP_NP_PRINT_DEFINED
#define LP_NP_PRINT_DEFINED
static inline void lp_np_print(LpArray arr) {
    printf("[");
    for (int64_t i = 0; i < arr.len; i++) {
        if (i > 0) printf(", ");
        double v = arr.data[i];
        if (v == (int64_t)v) printf("%" PRId64, (int64_t)v);
        else printf("%g", v);
    }
    printf("]\n");
}
#endif

/* Math helpers */
static inline int64_t lp_pow_int(int64_t base, int64_t exp) {
    int64_t result = 1;
    if (exp < 0) return 0;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

static inline int64_t lp_floordiv(int64_t a, int64_t b) {
    int64_t q, r;
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile (
        "cqto\n\t"
        "idivq %4\n\t"
        : "=a" (q), "=d" (r)
        : "a" (a), "d" (0), "r" (b)
    );
#else
    q = a / b;
    r = a % b;
#endif
    if ((r != 0) && ((r ^ b) < 0)) q--;
    return q;
}

static inline int64_t lp_mod(int64_t a, int64_t b) {
    int64_t r;
#if defined(__x86_64__) || defined(_M_X64)
    __asm__ volatile (
        "cqto\n\t"
        "idivq %3\n\t"
        : "=d" (r)
        : "a" (a), "d" (0), "r" (b)
        : "cc"
    );
#else
    r = a % b;
#endif
    if ((r != 0) && ((r ^ b) < 0)) r += b;
    return r;
}

/* JSON/Dict subscript helpers for LpVal */
static inline LpVal lp_val_getitem_str(LpVal obj, const char *key) {
    if (obj.type == LP_VAL_DICT) return lp_dict_get(obj.as.d, key);
    return lp_val_null();
}

static inline LpVal lp_val_getitem_int(LpVal obj, int64_t idx) {
    if (obj.type == LP_VAL_LIST) {
        if (idx >= 0 && idx < obj.as.l->len) return obj.as.l->items[idx];
    }
    return lp_val_null();
}

static inline int64_t lp_val_len(LpVal obj) {
    if (obj.type == LP_VAL_LIST) return obj.as.l->len;
    if (obj.type == LP_VAL_STRING) return strlen(obj.as.s);
    return 0;
}

/* String helpers */
static inline char *lp_str_concat(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char *)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

/* Cast helpers (int, float, str, bool) with _Generic support for LpVal */
static inline char* lp_str_from_val(LpVal v) {
    if (v.type == LP_VAL_INT) { char buf[32]; snprintf(buf, sizeof(buf), "%" PRId64, (int64_t)v.as.i); return strdup(buf); }
    if (v.type == LP_VAL_FLOAT) { char buf[32]; snprintf(buf, sizeof(buf), "%g", v.as.f); return strdup(buf); }
    if (v.type == LP_VAL_STRING) return strdup(v.as.s);
    if (v.type == LP_VAL_BOOL) return strdup(v.as.b ? "True" : "False");
    if (v.type == LP_VAL_NULL) return strdup("None");
    return strdup("[Object]");
}
static inline char* lp_str_from_int(int64_t v) { char buf[32]; snprintf(buf, sizeof(buf), "%" PRId64, (int64_t)v); return strdup(buf); }
static inline char* lp_str_from_float(double v) { char buf[32]; snprintf(buf, sizeof(buf), "%g", v); return strdup(buf); }
static inline char* lp_str_from_bool(int v) { return strdup(v ? "True" : "False"); }
static inline char* lp_str_from_str(const char *v) { return strdup(v); }

#define lp_str(x) _Generic((x), \
    LpVal: lp_str_from_val, \
    int64_t: lp_str_from_int, \
    double: lp_str_from_float, \
    float: lp_str_from_float, \
    char*: lp_str_from_str, \
    const char*: lp_str_from_str, \
    default: lp_str_from_int \
)(x)

static inline int64_t lp_int_from_int(int64_t v) { return v; }
static inline int64_t lp_int_from_float(double v) { return (int64_t)v; }

static inline int64_t lp_int_from_val(LpVal v) {
    if (v.type == LP_VAL_INT) return v.as.i;
    if (v.type == LP_VAL_FLOAT) return (int64_t)v.as.f;
    if (v.type == LP_VAL_STRING) return atoll(v.as.s);
    if (v.type == LP_VAL_BOOL) return v.as.b ? 1 : 0;
    return 0;
}

#define lp_int(x) _Generic((x), \
    LpVal: lp_int_from_val, \
    char*: atoll, \
    const char*: atoll, \
    double: lp_int_from_float, \
    float: lp_int_from_float, \
    default: lp_int_from_int \
)(x)

static inline double lp_float_from_int(int64_t v) { return (double)v; }
static inline double lp_float_from_float(double v) { return v; }

static inline double lp_float_from_val(LpVal v) {
    if (v.type == LP_VAL_INT) return (double)v.as.i;
    if (v.type == LP_VAL_FLOAT) return v.as.f;
    if (v.type == LP_VAL_STRING) return atof(v.as.s);
    if (v.type == LP_VAL_BOOL) return v.as.b ? 1.0 : 0.0;
    return 0.0;
}
#define lp_float(x) _Generic((x), \
    LpVal: lp_float_from_val, \
    char*: atof, \
    const char*: atof, \
    int64_t: lp_float_from_int, \
    int: lp_float_from_int, \
    default: lp_float_from_float \
)(x)

static inline int lp_bool_from_int(int64_t v) { return v != 0; }
static inline int lp_bool_from_float(double v) { return v != 0.0; }
static inline int lp_bool_from_ptr(const void *p) { return p != NULL; }

static inline int lp_bool_from_val(LpVal v) {
    if (v.type == LP_VAL_INT) return v.as.i != 0;
    if (v.type == LP_VAL_FLOAT) return v.as.f != 0.0;
    if (v.type == LP_VAL_STRING) return strlen(v.as.s) > 0;
    if (v.type == LP_VAL_BOOL) return v.as.b;
    if (v.type == LP_VAL_NULL) return 0;
    return 1;
}
#define lp_bool(x) _Generic((x), \
    LpVal: lp_bool_from_val, \
    int64_t: lp_bool_from_int, \
    int: lp_bool_from_int, \
    double: lp_bool_from_float, \
    float: lp_bool_from_float, \
    char*: lp_bool_from_ptr, \
    const char*: lp_bool_from_ptr, \
    void*: lp_bool_from_ptr, \
    default: lp_bool_from_int \
)(x)

#endif
