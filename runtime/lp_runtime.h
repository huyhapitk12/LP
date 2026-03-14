#ifndef LP_RUNTIME_H
#define LP_RUNTIME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>

#include "lp_compat.h"
#include "lp_dict.h"
#include "lp_set.h"
#include "lp_tuple.h"
#include "lp_io.h"
#include "lp_exception.h"
#include "lp_array.h"
#include "lp_args.h"

static inline void lp_args_extend_array(LpVarArgs *vargs, LpArray arr) {
    /* Special helper to expand an LpArray into LpVarArgs */
    if (!vargs || !arr.data || arr.shape[0] <= 0) return;
    for (int i = 0; i < arr.shape[0]; i++) {
        double *val_ptr = (double*)malloc(sizeof(double));
        if (!val_ptr) continue; /* Skip on allocation failure */
        *val_ptr = arr.data[i];
        lp_args_push(vargs, 1, val_ptr); /* 1 corresponds to LP_FLOAT generally but generic int */
    }
}

static inline LpArray lp_args_to_array(LpVarArgs *vargs) {
    LpArray arr;
    memset(&arr, 0, sizeof(LpArray));
    if (!vargs || vargs->count <= 0) return arr;
    arr.data = (double*)malloc(vargs->count * sizeof(double));
    if (!arr.data) return arr;
    arr.len = vargs->count;
    arr.cap = vargs->count;
    arr.shape[0] = vargs->count;
    arr.shape[1] = 0;
    arr.shape[2] = 0;
    arr.shape[3] = 0;
    for (int i = 0; i < vargs->count; i++) {
        /* Since we don't have deep run-time types mapped cleanly, fallback to float casts or pointer casting */
        if (vargs->types && vargs->types[i] == 1 && vargs->values[i]) {
            arr.data[i] = *((double*)vargs->values[i]);
        } else if (vargs->values[i]) {
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
    if (t && t->items) {
        for (int i = 0; i < t->count; i++) {
            if (i > 0) printf(", ");
            printf("%" PRId64, (int64_t)(intptr_t)t->items[i]);
        }
    }
    printf(")\n");
}

/* Print LpVal (dynamic JSON values) */
static inline void lp_print_val(LpVal v) {
    switch (v.type) {
        case LP_VAL_INT:    printf("%" PRId64 "\n", (int64_t)v.as.i); break;
        case LP_VAL_FLOAT:  printf("%g\n", v.as.f); break;
        case LP_VAL_STRING: if (v.as.s) printf("%s\n", v.as.s); else printf("\n"); break;
        case LP_VAL_BOOL:   printf("%s\n", v.as.b ? "True" : "False"); break;
        case LP_VAL_NULL:   printf("None\n"); break;
        case LP_VAL_DICT:   if (v.as.d) lp_dict_print(v.as.d); printf("\n"); break;
        case LP_VAL_LIST:   if (v.as.l) lp_list_print(v.as.l); printf("\n"); break;
    }
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
    if (arr.data) {
        for (int64_t i = 0; i < arr.len; i++) {
            if (i > 0) printf(", ");
            double v = arr.data[i];
            if (v == (int64_t)v) printf("%" PRId64, (int64_t)v);
            else printf("%g", v);
        }
    }
    printf("]\n");
}
#endif

/* Math helpers */
static inline int64_t lp_pow_int(int64_t base, int64_t exp) {
    int64_t result = 1;
    if (exp < 0) return 0;
    if (base == 0) return 0;
    if (base == 1) return 1;
    if (base == -1) return (exp % 2 == 0) ? 1 : -1;
    while (exp > 0) {
        if (exp & 1) {
            /* Check for overflow before multiplication */
            if (result > INT64_MAX / base || result < INT64_MIN / base) {
                return (base > 0) ? INT64_MAX : INT64_MIN;  /* Overflow */
            }
            result *= base;
        }
        if (exp > 1) {  /* Don't square on last iteration */
            /* Check for overflow before squaring */
            if (base > INT64_MAX / base || base < INT64_MIN / base) {
                return (base > 0) ? INT64_MAX : INT64_MIN;  /* Overflow */
            }
            base *= base;
        }
        exp >>= 1;
    }
    return result;
}

static inline int64_t lp_floordiv(int64_t a, int64_t b) {
    int64_t q, r;
    /* Division by zero check */
    if (b == 0) return 0;  /* Return 0 on error, caller should handle */
    /* Use portable C code for correctness - compiler will optimize */
    q = a / b;
    r = a % b;
    if ((r != 0) && ((r ^ b) < 0)) q--;
    return q;
}

static inline int64_t lp_mod(int64_t a, int64_t b) {
    int64_t r;
    /* Division by zero check */
    if (b == 0) return 0;  /* Return 0 on error, caller should handle */
    /* Use portable C code for correctness - compiler will optimize */
    r = a % b;
    if ((r != 0) && ((r ^ b) < 0)) r += b;
    return r;
}

/* Math operations on LpVal */
static inline LpVal lp_val_add(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return lp_val_int(a.as.i + b.as.i);
    /* FIX: Chỉ cho phép tính float nếu CẢ HAI đều là kiểu số (int hoặc float) */
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return lp_val_float(f_a + f_b);
    }
    if (a.type == LP_VAL_STRING && b.type == LP_VAL_STRING) {
        char *str = (char*)malloc(strlen(a.as.s) + strlen(b.as.s) + 1);
        if (!str) return lp_val_null(); /* An toàn OOM */
        strcpy(str, a.as.s);
        strcat(str, b.as.s);
        LpVal v; v.type = LP_VAL_STRING; v.as.s = str; return v;
    }
    return lp_val_null();
}

static inline LpVal lp_val_sub(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return lp_val_int(a.as.i - b.as.i);
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return lp_val_float(f_a - f_b);
    }
    return lp_val_null();
}

static inline LpVal lp_val_mul(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return lp_val_int(a.as.i * b.as.i);
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return lp_val_float(f_a * f_b);
    }
    return lp_val_null();
}

static inline LpVal lp_val_div(LpVal a, LpVal b) {
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        if (f_b == 0.0) return lp_val_null(); /* or throw exception */
        return lp_val_float(f_a / f_b);
    }
    return lp_val_null();
}

static inline LpVal lp_val_floordiv(LpVal a, LpVal b) {
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        int64_t i_a = (a.type == LP_VAL_INT) ? a.as.i : (int64_t)a.as.f;
        int64_t i_b = (b.type == LP_VAL_INT) ? b.as.i : (int64_t)b.as.f;
        if (i_b == 0) return lp_val_null();
        return lp_val_int(lp_floordiv(i_a, i_b));
    }
    return lp_val_null();
}

static inline LpVal lp_val_mod(LpVal a, LpVal b) {
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        int64_t i_a = (a.type == LP_VAL_INT) ? a.as.i : (int64_t)a.as.f;
        int64_t i_b = (b.type == LP_VAL_INT) ? b.as.i : (int64_t)b.as.f;
        if (i_b == 0) return lp_val_null();
        return lp_val_int(lp_mod(i_a, i_b));
    }
    return lp_val_null();
}

static inline int lp_val_eq(LpVal a, LpVal b) {
    if (a.type != b.type) {
        if ((a.type == LP_VAL_INT || a.type == LP_VAL_FLOAT) && (b.type == LP_VAL_INT || b.type == LP_VAL_FLOAT)) {
            double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
            double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
            return f_a == f_b;
        }
        return 0;
    }
    switch (a.type) {
        case LP_VAL_INT: return a.as.i == b.as.i;
        case LP_VAL_FLOAT: return a.as.f == b.as.f;
        case LP_VAL_STRING: return (a.as.s && b.as.s) ? (strcmp(a.as.s, b.as.s) == 0) : (a.as.s == b.as.s);
        case LP_VAL_BOOL: return a.as.b == b.as.b;
        case LP_VAL_NULL: return 1;
        default: return 0; /* pointer comparison for dict/list isn't full deep eq yet */
    }
}

static inline int lp_val_neq(LpVal a, LpVal b) { return !lp_val_eq(a, b); }

static inline int lp_val_lt(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return a.as.i < b.as.i;
    if (a.type == LP_VAL_FLOAT || b.type == LP_VAL_FLOAT) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return f_a < f_b;
    }
    return 0;
}

static inline int lp_val_gt(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return a.as.i > b.as.i;
    if (a.type == LP_VAL_FLOAT || b.type == LP_VAL_FLOAT) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return f_a > f_b;
    }
    return 0;
}

static inline int lp_val_lte(LpVal a, LpVal b) { 
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return a.as.i <= b.as.i;
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return f_a <= f_b;
    }
    return 0; 
}

static inline int lp_val_gte(LpVal a, LpVal b) { 
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT) return a.as.i >= b.as.i;
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) && 
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return f_a >= f_b;
    }
    return 0; 
}

/* Sorting and searching helpers */
static inline int lp_val_cmp(const void *a, const void *b) {
    LpVal *va = (LpVal*)a;
    LpVal *vb = (LpVal*)b;
    if (lp_val_lt(*va, *vb)) return -1;
    if (lp_val_gt(*va, *vb)) return 1;
    return 0;
}

static inline void lp_list_sort(LpList *l) {
    if (l && l->items && l->len > 1) {
        qsort(l->items, l->len, sizeof(LpVal), lp_val_cmp);
    }
}

static inline int64_t lp_list_binary_search(LpList *l, LpVal v) {
    if (!l || !l->items || l->len == 0) return -1;
    int64_t low = 0;
    int64_t high = l->len - 1;
    while (low <= high) {
        int64_t mid = low + (high - low) / 2;
        if (lp_val_eq(l->items[mid], v)) return mid;
        if (lp_val_lt(l->items[mid], v)) low = mid + 1;
        else high = mid - 1;
    }
    return -1;
}

/* JSON/Dict subscript helpers for LpVal */
static inline LpVal lp_val_getitem_str(LpVal obj, const char *key) {
    if (obj.type == LP_VAL_DICT && obj.as.d) return lp_dict_get(obj.as.d, key);
    return lp_val_null();
}

static inline LpVal lp_val_getitem_int(LpVal obj, int64_t idx) {
    if (obj.type == LP_VAL_LIST && obj.as.l && obj.as.l->items) {
        if (idx >= 0 && idx < obj.as.l->len) return obj.as.l->items[idx];
    }
    return lp_val_null();
}

static inline int64_t lp_val_len(LpVal obj) {
    if (obj.type == LP_VAL_LIST && obj.as.l) return obj.as.l->len;
    if (obj.type == LP_VAL_STRING && obj.as.s) return (int64_t)strlen(obj.as.s);
    return 0;
}


/* String helpers */
static inline char *lp_str_concat(const char *a, const char *b) {
    if (!a || !b) return NULL;
    size_t la = strlen(a), lb = strlen(b);
    char *r = (char *)malloc(la + lb + 1);
    if (!r) return NULL;
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

static inline int64_t lp_int_from_str(const char *v) { return atoll(v); }

#define lp_int(x) _Generic((x), \
    LpVal: lp_int_from_val, \
    char*: lp_int_from_str, \
    const char*: lp_int_from_str, \
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

static inline void lp_val_set_item(LpVal obj, LpVal key, LpVal val) {
    if (obj.type == LP_VAL_DICT) {
        if (key.type == LP_VAL_STRING) {
            lp_dict_set(obj.as.d, key.as.s, val);
        } else {
            char* s = lp_str(key);
            lp_dict_set(obj.as.d, s, val);
            free(s);
        }
    } else if (obj.type == LP_VAL_LIST) {
        int64_t idx = lp_int(key);
        if (idx >= 0 && idx < obj.as.l->len) {
            obj.as.l->items[idx] = val;
        }
    }
}

#endif
