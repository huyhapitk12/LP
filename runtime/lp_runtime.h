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

/* C11 Generic print for unknown/inferred types (like object properties) */
#define lp_print_generic(x) _Generic((x), \
    double: lp_print_float, \
    float: lp_print_float, \
    char *: lp_print_str, \
    const char *: lp_print_str, \
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

/* String helpers */
static inline char *lp_str_concat(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char *)malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

#endif
