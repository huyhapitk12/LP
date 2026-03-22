#ifndef LP_RUNTIME_H
#define LP_RUNTIME_H

/* Feature-test macros must appear before any system header to expose
 * C11/POSIX extensions like aligned_alloc on glibc */
#ifndef _DEFAULT_SOURCE
#  define _DEFAULT_SOURCE
#endif
#ifndef _ISOC11_SOURCE
#  define _ISOC11_SOURCE
#endif

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
    /* Division by zero check */
    if (__builtin_expect(b == 0, 0)) return 0;
    /* Fast path: non-negative dividend with positive divisor (most common in CP) */
    if (__builtin_expect(a >= 0 && b > 0, 1)) return a / b;
    /* General case: Python-style floor division */
    int64_t q = a / b;
    int64_t r = a % b;
    if ((r != 0) && ((r ^ b) < 0)) q--;
    return q;
}

static inline int64_t lp_mod(int64_t a, int64_t b) {
    /* Division by zero check */
    if (__builtin_expect(b == 0, 0)) return 0;
    /* Fast path: non-negative dividend with positive divisor (most common in CP) */
    if (__builtin_expect(a >= 0 && b > 0, 1)) return a % b;
    /* General case: Python-style modulo */
    int64_t r = a % b;
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
        /* SECURITY FIX: Use safe string operations */
        size_t len_a = strlen(a.as.s);
        size_t len_b = strlen(b.as.s);
        /* Check for potential overflow */
        if (len_a > SIZE_MAX - len_b - 1) {
            return lp_val_null(); /* Would overflow */
        }
        char *str = (char*)malloc(len_a + len_b + 1);
        if (!str) return lp_val_null(); /* An toàn OOM */
        /* Use memcpy instead of strcpy/strcat for safety */
        memcpy(str, a.as.s, len_a);
        memcpy(str + len_a, b.as.s, len_b);
        str[len_a + len_b] = '\0';
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

/* ================================================================
 * TIMSORT for LpVal arrays - O(n) best case, O(n log n) worst case
 * Stable sort, optimized for partially ordered data
 * ================================================================ */

#define LP_VAL_TIMSORT_MIN_RUN 32

/* Insertion sort for small runs of LpVal */
static inline void _lp_val_timsort_insertion_sort(LpVal *arr, int64_t left, int64_t right) {
    for (int64_t i = left + 1; i <= right; i++) {
        LpVal key = arr[i];
        int64_t j = i - 1;
        while (j >= left && lp_val_gt(arr[j], key)) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/* Merge two sorted runs of LpVal */
static inline void _lp_val_timsort_merge(LpVal *arr, int64_t l, int64_t m, int64_t r, LpVal *temp) {
    int64_t len1 = m - l + 1;
    int64_t len2 = r - m;
    
    /* Copy to temp array */
    for (int64_t i = 0; i < len1; i++)
        temp[i] = arr[l + i];
    for (int64_t i = 0; i < len2; i++)
        temp[len1 + i] = arr[m + 1 + i];
    
    int64_t i = 0, j = 0, k = l;
    
    /* Stable merge: use <= to maintain stability */
    while (i < len1 && j < len2) {
        if (lp_val_lte(temp[i], temp[j])) {
            arr[k++] = temp[i++];
        } else {
            arr[k++] = temp[j++];
        }
    }
    
    while (i < len1)
        arr[k++] = temp[i++];
    while (j < len2)
        arr[k++] = temp[j++];
}

/* Calculate minimum run length */
static inline int64_t _lp_val_timsort_min_run_length(int64_t n) {
    int64_t r = 0;
    while (n >= LP_VAL_TIMSORT_MIN_RUN) {
        r |= (n & 1);
        n >>= 1;
    }
    return n + r;
}

/* Find natural run in LpVal array */
static inline int64_t _lp_val_timsort_count_run(LpVal *arr, int64_t left, int64_t right) {
    if (left >= right) return 1;
    
    int64_t run_len = 1;
    
    /* Check if descending */
    if (lp_val_gt(arr[left], arr[left + 1])) {
        /* Descending run - find end and reverse */
        while (left + run_len < right && lp_val_gt(arr[left + run_len], arr[left + run_len + 1])) {
            run_len++;
        }
        /* Reverse */
        for (int64_t i = 0; i < (run_len + 1) / 2; i++) {
            LpVal tmp = arr[left + i];
            arr[left + i] = arr[left + run_len - i];
            arr[left + run_len - i] = tmp;
        }
        return run_len + 1;
    } else {
        /* Ascending run */
        while (left + run_len < right && lp_val_lte(arr[left + run_len], arr[left + run_len + 1])) {
            run_len++;
        }
        return run_len + 1;
    }
}

/* Main TimSort for LpVal array */
static inline void _lp_val_timsort(LpVal *arr, int64_t n, LpVal *temp) {
    if (n < 2) return;
    
    int64_t min_run = _lp_val_timsort_min_run_length(n);
    
    /* Sort individual runs */
    int64_t left = 0;
    while (left < n) {
        int64_t run_len = _lp_val_timsort_count_run(arr, left, n - 1);
        int64_t right = left + run_len - 1;
        
        if (run_len < min_run) {
            int64_t extend = (left + min_run - 1) < (n - 1) ? (left + min_run - 1) : (n - 1);
            _lp_val_timsort_insertion_sort(arr, left, extend);
            right = extend;
        }
        
        left = right + 1;
    }
    
    /* Merge runs bottom-up */
    for (int64_t size = min_run; size < n; size = 2 * size) {
        for (int64_t l = 0; l < n; l += 2 * size) {
            int64_t m = l + size - 1;
            int64_t r = (l + 2 * size - 1) < (n - 1) ? (l + 2 * size - 1) : (n - 1);
            
            if (m < r) {
                _lp_val_timsort_merge(arr, l, m, r, temp);
            }
        }
    }
}

/* Comparison function for qsort fallback */
static inline int lp_val_cmp(const void *a, const void *b) {
    LpVal *va = (LpVal*)a;
    LpVal *vb = (LpVal*)b;
    if (lp_val_lt(*va, *vb)) return -1;
    if (lp_val_gt(*va, *vb)) return 1;
    return 0;
}

/* TimSort wrapper for LpList */
static inline void lp_list_sort(LpList *l) {
    if (!l || !l->items || l->len <= 1) return;
    
    /* Allocate temp buffer */
    LpVal *temp = (LpVal*)malloc(l->len * sizeof(LpVal));
    if (temp) {
        _lp_val_timsort(l->items, l->len, temp);
        free(temp);
    } else {
        /* Fallback to qsort if allocation fails */
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

/* === Runtime Type Checking Helpers for Union Types === */
/* These functions are used for 'is' and 'isinstance' checks with union types */

/* Check if LpVal is int */
static inline int lp_val_is_int(LpVal v) {
    return v.type == LP_VAL_INT;
}

/* Check if LpVal is float */
static inline int lp_val_is_float(LpVal v) {
    return v.type == LP_VAL_FLOAT;
}

/* Check if LpVal is string */
static inline int lp_val_is_str(LpVal v) {
    return v.type == LP_VAL_STRING;
}

/* Check if LpVal is bool */
static inline int lp_val_is_bool(LpVal v) {
    return v.type == LP_VAL_BOOL;
}

/* Check if LpVal is None/null */
static inline int lp_val_is_none(LpVal v) {
    return v.type == LP_VAL_NULL;
}

/* Check if LpVal is list */
static inline int lp_val_is_list(LpVal v) {
    return v.type == LP_VAL_LIST;
}

/* Check if LpVal is dict */
static inline int lp_val_is_dict(LpVal v) {
    return v.type == LP_VAL_DICT;
}

/* Check if LpVal is numeric (int or float) */
static inline int lp_val_is_numeric(LpVal v) {
    return v.type == LP_VAL_INT || v.type == LP_VAL_FLOAT;
}

/* Generic isinstance for LpVal - compares against type name string */
static inline int lp_val_isinstance(LpVal v, const char *type_name) {
    if (!type_name) return 0;
    
    if (strcmp(type_name, "int") == 0) return v.type == LP_VAL_INT;
    if (strcmp(type_name, "float") == 0) return v.type == LP_VAL_FLOAT;
    if (strcmp(type_name, "str") == 0) return v.type == LP_VAL_STRING;
    if (strcmp(type_name, "bool") == 0) return v.type == LP_VAL_BOOL;
    if (strcmp(type_name, "None") == 0 || strcmp(type_name, "none") == 0) return v.type == LP_VAL_NULL;
    if (strcmp(type_name, "list") == 0) return v.type == LP_VAL_LIST;
    if (strcmp(type_name, "dict") == 0) return v.type == LP_VAL_DICT;
    if (strcmp(type_name, "numeric") == 0) return lp_val_is_numeric(v);
    
    return 0;
}

/* Check if value matches any type in a union type string (e.g., "int|str|float") */
static inline int lp_val_matches_union(LpVal v, const char *union_types) {
    if (!union_types) return 0;
    
    /* Simple check using strstr - more portable than strtok_r */
    if (strstr(union_types, "int") && v.type == LP_VAL_INT) return 1;
    if (strstr(union_types, "float") && v.type == LP_VAL_FLOAT) return 1;
    if (strstr(union_types, "str") && v.type == LP_VAL_STRING) return 1;
    if (strstr(union_types, "bool") && v.type == LP_VAL_BOOL) return 1;
    if (strstr(union_types, "None") && v.type == LP_VAL_NULL) return 1;
    if (strstr(union_types, "dict") && v.type == LP_VAL_DICT) return 1;
    if (strstr(union_types, "list") && v.type == LP_VAL_LIST) return 1;
    if (strstr(union_types, "numeric") && (v.type == LP_VAL_INT || v.type == LP_VAL_FLOAT)) return 1;
    
    return 0;
}

/* Type name string for LpVal */
static inline const char* lp_val_type_name(LpVal v) {
    switch (v.type) {
        case LP_VAL_INT:    return "int";
        case LP_VAL_FLOAT:  return "float";
        case LP_VAL_STRING: return "str";
        case LP_VAL_BOOL:   return "bool";
        case LP_VAL_NULL:   return "None";
        case LP_VAL_DICT:   return "dict";
        case LP_VAL_LIST:   return "list";
        default:            return "unknown";
    }
}

#endif
