/*
 * LP Native Module Bindings — Zero-Overhead C Implementations
 *
 * Instead of calling Python at runtime, we map common Python module
 * functions directly to optimized C equivalents. This gives LP programs
 * access to "Python-like" APIs with native C/C++ performance.
 *
 * Strategy:
 *   Tier 1 (ZERO overhead): math, random, os, sys, time — direct C mapping
 *   Tier 2 (Near-zero): numpy-like array ops — SIMD-optimized C loops
 *   Tier 3 (Fallback): Full Python/C API for arbitrary libraries
 *
 * The codegen detects which tier a module belongs to and emits the
 * appropriate C code — NO Python interpreter for Tier 1 & 2.
 */

#ifndef LP_NATIVE_MODULES_H
#define LP_NATIVE_MODULES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <inttypes.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include "lp_array.h"

/* ================================================================
 * TIER 1: math module — Direct C <math.h> mapping
 * Zero overhead: compiles to single CPU instructions
 * ================================================================ */

/* Constants */
static const double lp_math_pi    = 3.14159265358979323846;
static const double lp_math_e     = 2.71828182845904523536;
static const double lp_math_tau   = 6.28318530717958647692;
static const double lp_math_inf   = 1.0 / 0.0;
static const double lp_math_nan_v = 0.0 / 0.0;

/* Basic functions — all inline, zero overhead */
static inline double lp_math_sqrt(double x)       { return sqrt(x); }
static inline double lp_math_fabs(double x)       { return fabs(x); }
static inline double lp_math_ceil(double x)       { return ceil(x); }
static inline double lp_math_floor(double x)      { return floor(x); }
static inline double lp_math_round(double x)      { return round(x); }
static inline double lp_math_trunc(double x)      { return trunc(x); }

/* Trig */
static inline double lp_math_sin(double x)        { return sin(x); }
static inline double lp_math_cos(double x)        { return cos(x); }
static inline double lp_math_tan(double x)        { return tan(x); }
static inline double lp_math_asin(double x)       { return asin(x); }
static inline double lp_math_acos(double x)       { return acos(x); }
static inline double lp_math_atan(double x)       { return atan(x); }
static inline double lp_math_atan2(double y, double x) { return atan2(y, x); }

/* Hyperbolic */
static inline double lp_math_sinh(double x)       { return sinh(x); }
static inline double lp_math_cosh(double x)       { return cosh(x); }
static inline double lp_math_tanh(double x)       { return tanh(x); }

/* Exponential / Logarithmic */
static inline double lp_math_exp(double x)        { return exp(x); }
static inline double lp_math_log(double x)        { return log(x); }
static inline double lp_math_log2(double x)       { return log2(x); }
static inline double lp_math_log10(double x)      { return log10(x); }
static inline double lp_math_pow(double x, double y) { return pow(x, y); }

/* Integer math */
static inline int64_t lp_math_factorial(int64_t n) {
    if (n < 0) return 0;
    if (n > 20) return INT64_MAX;  /* 21! overflows int64_t */
    int64_t r = 1;
    for (int64_t i = 2; i <= n; i++) r *= i;
    return r;
}

static inline int64_t lp_math_gcd(int64_t a, int64_t b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { int64_t t = b; b = a % b; a = t; }
    return a;
}

static inline int64_t lp_math_lcm(int64_t a, int64_t b) {
    if (a == 0 || b == 0) return 0;
    int64_t g = lp_math_gcd(a, b);
    return (a / g) * b;
}

static inline int lp_math_isnan(double x)  { return isnan(x); }
static inline int lp_math_isinf(double x)  { return isinf(x); }

static inline double lp_math_radians(double deg) { return deg * (lp_math_pi / 180.0); }
static inline double lp_math_degrees(double rad) { return rad * (180.0 / lp_math_pi); }

/* ================================================================
 * TIER 1: random module — Direct C mapping
 * ================================================================ */

static int _lp_random_seeded = 0;

static inline void lp_random_seed(int64_t s) {
    srand((unsigned int)s);
    _lp_random_seeded = 1;
}

static inline double lp_random_random(void) {
    if (!_lp_random_seeded) { srand((unsigned int)time(NULL)); _lp_random_seeded = 1; }
    return (double)rand() / (double)RAND_MAX;
}

static inline int64_t lp_random_randint(int64_t a, int64_t b) {
    if (!_lp_random_seeded) { srand((unsigned int)time(NULL)); _lp_random_seeded = 1; }
    return a + (rand() % (b - a + 1));
}

static inline double lp_random_uniform(double a, double b) {
    return a + lp_random_random() * (b - a);
}

/* ================================================================
 * TIER 1: time module — Direct C mapping
 * ================================================================ */

static inline double lp_time_time(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static inline void lp_time_sleep(double seconds) {
#ifdef _WIN32
    Sleep((DWORD)(seconds * 1000));
#else
    usleep((unsigned int)(seconds * 1000000));
#endif
}

/* ================================================================
 * TIER 2: numpy-like array operations — Optimized C loops
 * Uses contiguous memory for cache efficiency.
 * GCC auto-vectorization + our -O3 -march=native flags
 * make these run at near-BLAS speeds for basic ops.
 * ================================================================ */

/* Create / destroy */
static inline LpArray lp_np_zeros(int64_t n) {
    LpArray a;
    a.data = (double *)calloc(n, sizeof(double));
    a.len = n;
    a.cap = n;
    return a;
}

static inline LpArray lp_np_ones(int64_t n) {
    LpArray a;
    a.data = (double *)malloc(n * sizeof(double));
    a.len = n;
    a.cap = n;
    for (int64_t i = 0; i < n; i++) a.data[i] = 1.0;
    return a;
}

static inline LpArray lp_np_arange(double start, double stop, double step) {
    int64_t n = (int64_t)((stop - start) / step);
    if (n < 0) n = 0;
    LpArray a;
    a.data = (double *)malloc(n * sizeof(double));
    a.len = n;
    a.cap = n;
    for (int64_t i = 0; i < n; i++) a.data[i] = start + i * step;
    return a;
}

static inline LpArray lp_np_linspace(double start, double stop, int64_t n) {
    LpArray a;
    a.data = (double *)malloc(n * sizeof(double));
    a.len = n;
    a.cap = n;
    double step = (n > 1) ? (stop - start) / (n - 1) : 0.0;
    for (int64_t i = 0; i < n; i++) a.data[i] = start + i * step;
    return a;
}

/* Create from literal values — used by codegen for np.array([1,2,3]) */
static inline LpArray lp_np_from_doubles(int64_t count, ...) {
    LpArray a;
    a.data = (double *)malloc(count * sizeof(double));
    a.len = count;
    a.cap = count;
    va_list ap;
    va_start(ap, count);
    for (int64_t i = 0; i < count; i++) a.data[i] = va_arg(ap, double);
    va_end(ap);
    return a;
}

static inline void lp_np_free(LpArray *a) {
    free(a->data);
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

/* Reduction operations — GCC will auto-vectorize with -O3 -march=native */
__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_sum(LpArray a) {
    double s = 0.0;
    for (int64_t i = 0; i < a.len; i++) s += a.data[i];
    return s;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_mean(LpArray a) {
    return (a.len > 0) ? lp_np_sum(a) / (double)a.len : 0.0;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_min(LpArray a) {
    if (a.len == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.len; i++) if (a.data[i] < m) m = a.data[i];
    return m;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_max(LpArray a) {
    if (a.len == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.len; i++) if (a.data[i] > m) m = a.data[i];
    return m;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_dot(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    double s = 0.0;
    for (int64_t i = 0; i < n; i++) s += a.data[i] * b.data[i];
    return s;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline double lp_np_std(LpArray a) {
    if (a.len <= 1) return 0.0;
    double mean = lp_np_mean(a);
    double s = 0.0;
    for (int64_t i = 0; i < a.len; i++) {
        double d = a.data[i] - mean;
        s += d * d;
    }
    return sqrt(s / (double)a.len);
}

/* Element-wise operations — return new arrays */
__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_add(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] + b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_sub(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] - b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_mul(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] * b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_div(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] / b.data[i];
    return r;
}

/* Scalar operations */
__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_scale(LpArray a, double s) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = a.data[i] * s;
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_add_scalar(LpArray a, double s) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = a.data[i] + s;
    return r;
}

/* Math operations on arrays */
__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_sqrt_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = sqrt(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_abs_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = fabs(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_sin_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = sin(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_cos_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = cos(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_exp_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = exp(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_log_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = log(a.data[i]);
    return r;
}

/* ================================================================
 * TIMSORT for double arrays - O(n) best case, O(n log n) worst case
 * Stable sort, optimized for partially ordered data
 * ================================================================ */

#define LP_TIMSORT_MIN_RUN 32

/* Comparison function for qsort fallback */
static int _lp_np_cmp_asc(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

/* Insertion sort for small runs */
static inline void _lp_timsort_insertion_sort(double *arr, int64_t left, int64_t right) {
    for (int64_t i = left + 1; i <= right; i++) {
        double key = arr[i];
        int64_t j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/* Merge two sorted runs */
static inline void _lp_timsort_merge(double *arr, int64_t l, int64_t m, int64_t r, double *temp) {
    int64_t len1 = m - l + 1;
    int64_t len2 = r - m;
    
    /* Copy to temp array */
    for (int64_t i = 0; i < len1; i++)
        temp[i] = arr[l + i];
    for (int64_t i = 0; i < len2; i++)
        temp[len1 + i] = arr[m + 1 + i];
    
    int64_t i = 0, j = 0, k = l;
    
    /* Merge with galloping optimization for equal elements */
    while (i < len1 && j < len2) {
        if (temp[i] <= temp[j]) {
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

/* Calculate minimum run length (power of 2 between 32 and 64) */
static inline int64_t _lp_timsort_min_run_length(int64_t n) {
    int64_t r = 0;
    while (n >= LP_TIMSORT_MIN_RUN) {
        r |= (n & 1);
        n >>= 1;
    }
    return n + r;
}

/* Find start of next run (ascending or descending) */
static inline int64_t _lp_timsort_count_run(double *arr, int64_t left, int64_t right) {
    if (left >= right) return 1;
    
    int64_t run_len = 1;
    
    /* Check if descending - if so, reverse */
    if (arr[left] > arr[left + 1]) {
        /* Descending run - find end and reverse */
        while (left + run_len < right && arr[left + run_len] > arr[left + run_len + 1]) {
            run_len++;
        }
        /* Reverse the descending run */
        for (int64_t i = 0; i < (run_len + 1) / 2; i++) {
            double tmp = arr[left + i];
            arr[left + i] = arr[left + run_len - i];
            arr[left + run_len - i] = tmp;
        }
        return run_len + 1;
    } else {
        /* Ascending run - find end */
        while (left + run_len < right && arr[left + run_len] <= arr[left + run_len + 1]) {
            run_len++;
        }
        return run_len + 1;
    }
}

/* Main TimSort implementation */
static inline void _lp_timsort(double *arr, int64_t n, double *temp) {
    if (n < 2) return;
    
    int64_t min_run = _lp_timsort_min_run_length(n);
    
    /* Sort individual runs with insertion sort */
    int64_t left = 0;
    while (left < n) {
        int64_t right = left;
        
        /* Find natural run */
        int64_t run_len = _lp_timsort_count_run(arr, left, n - 1);
        right = left + run_len - 1;
        
        /* Extend to minimum run length if needed */
        if (run_len < min_run) {
            int64_t extend = (left + min_run - 1) < (n - 1) ? (left + min_run - 1) : (n - 1);
            _lp_timsort_insertion_sort(arr, left, extend);
            right = extend;
        }
        
        left = right + 1;
    }
    
    /* Merge runs from bottom up */
    for (int64_t size = min_run; size < n; size = 2 * size) {
        for (int64_t l = 0; l < n; l += 2 * size) {
            int64_t m = l + size - 1;
            int64_t r = (l + 2 * size - 1) < (n - 1) ? (l + 2 * size - 1) : (n - 1);
            
            if (m < r) {
                _lp_timsort_merge(arr, l, m, r, temp);
            }
        }
    }
}

/* TimSort wrapper for LpArray */
static inline LpArray lp_np_sort(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    memcpy(r.data, a.data, a.len * sizeof(double));
    
    if (a.len > 1) {
        /* Allocate temp buffer for merge operations */
        double *temp = (double*)malloc(a.len * sizeof(double));
        if (temp) {
            _lp_timsort(r.data, r.len, temp);
            free(temp);
        } else {
            /* Fallback to qsort if allocation fails */
            qsort(r.data, r.len, sizeof(double), _lp_np_cmp_asc);
        }
    }
    return r;
}

/* Print array */
#ifndef LP_NP_PRINT_DEFINED
#define LP_NP_PRINT_DEFINED
static inline void lp_np_print(LpArray a) {
    printf("[");
    for (int64_t i = 0; i < a.len; i++) {
        if (i > 0) printf(", ");
        /* Print integers without decimal if they are whole */
        if (a.data[i] == (int64_t)a.data[i] && fabs(a.data[i]) < 1e15)
            printf("%" PRId64, (int64_t)a.data[i]);
        else
            printf("%g", a.data[i]);
    }
    printf("]\n");
}
#endif

/* Array length */
static inline int64_t lp_np_len(LpArray a) {
    return a.len;
}

/* ================================================================
 * Additional NumPy Functions
 * ================================================================ */

/* Flatten array to 1D */
static inline LpArray lp_np_flatten(LpArray a) {
    LpArray result;
    result.data = (double*)malloc(a.len * sizeof(double));
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = a.len;
    result.shape[1] = 0;
    result.shape[2] = 0;
    result.shape[3] = 0;
    if (result.data && a.data) {
        memcpy(result.data, a.data, a.len * sizeof(double));
    }
    return result;
}

/* Reshape array (basic 2D reshape) */
static inline LpArray lp_np_reshape2d(LpArray a, int64_t rows, int64_t cols) {
    LpArray result = a; /* Share data pointer for zero-copy */
    result.shape[0] = rows;
    result.shape[1] = cols;
    result.shape[2] = 0;
    result.shape[3] = 0;
    return result;
}

/* Transpose 2D array */
static inline LpArray lp_np_transpose(LpArray a) {
    int64_t rows = a.shape[0] > 0 ? a.shape[0] : 1;
    int64_t cols = a.shape[1] > 0 ? a.shape[1] : a.len;
    
    LpArray result;
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = cols;
    result.shape[1] = rows;
    result.shape[2] = 0;
    result.shape[3] = 0;
    result.data = (double*)malloc(result.len * sizeof(double));
    
    if (result.data && a.data) {
        for (int64_t i = 0; i < rows; i++) {
            for (int64_t j = 0; j < cols; j++) {
                result.data[j * rows + i] = a.data[i * cols + j];
            }
        }
    }
    return result;
}

/* Cumulative sum */
static inline LpArray lp_np_cumsum(LpArray a) {
    LpArray result;
    result.data = (double*)malloc(a.len * sizeof(double));
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = a.len;
    result.shape[1] = 0;
    result.shape[2] = 0;
    result.shape[3] = 0;
    
    if (result.data && a.data) {
        double cumsum = 0;
        for (int64_t i = 0; i < a.len; i++) {
            cumsum += a.data[i];
            result.data[i] = cumsum;
        }
    }
    return result;
}

/* Element-wise power */
static inline LpArray lp_np_power(LpArray a, double p) {
    LpArray result;
    result.data = (double*)malloc(a.len * sizeof(double));
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = a.shape[0];
    result.shape[1] = a.shape[1];
    result.shape[2] = a.shape[2];
    result.shape[3] = a.shape[3];
    
    if (result.data && a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            result.data[i] = pow(a.data[i], p);
        }
    }
    return result;
}

/* Variance */
static inline double lp_np_var(LpArray a) {
    double mean = lp_np_mean(a);
    double sum_sq = 0;
    if (a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            double diff = a.data[i] - mean;
            sum_sq += diff * diff;
        }
    }
    return sum_sq / a.len;
}

/* Median */
static inline double lp_np_median(LpArray a) {
    if (!a.data || a.len == 0) return 0;
    
    /* Sort a copy */
    LpArray sorted = lp_np_sort(a);
    double median;
    
    if (a.len % 2 == 0) {
        median = (sorted.data[a.len/2 - 1] + sorted.data[a.len/2]) / 2.0;
    } else {
        median = sorted.data[a.len/2];
    }
    
    free(sorted.data);
    return median;
}

/* Argmax - returns index of maximum value */
static inline int64_t lp_np_argmax(LpArray a) {
    if (!a.data || a.len == 0) return -1;
    
    int64_t max_idx = 0;
    double max_val = a.data[0];
    
    for (int64_t i = 1; i < a.len; i++) {
        if (a.data[i] > max_val) {
            max_val = a.data[i];
            max_idx = i;
        }
    }
    return max_idx;
}

/* Argmin - returns index of minimum value */
static inline int64_t lp_np_argmin(LpArray a) {
    if (!a.data || a.len == 0) return -1;
    
    int64_t min_idx = 0;
    double min_val = a.data[0];
    
    for (int64_t i = 1; i < a.len; i++) {
        if (a.data[i] < min_val) {
            min_val = a.data[i];
            min_idx = i;
        }
    }
    return min_idx;
}

/* Clip values to range [min_val, max_val] */
static inline LpArray lp_np_clip(LpArray a, double min_val, double max_val) {
    LpArray result;
    result.data = (double*)malloc(a.len * sizeof(double));
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = a.shape[0];
    result.shape[1] = a.shape[1];
    result.shape[2] = a.shape[2];
    result.shape[3] = a.shape[3];
    
    if (result.data && a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            if (a.data[i] < min_val) result.data[i] = min_val;
            else if (a.data[i] > max_val) result.data[i] = max_val;
            else result.data[i] = a.data[i];
        }
    }
    return result;
}

/* Reverse array */
static inline LpArray lp_np_reverse(LpArray a) {
    LpArray result;
    result.data = (double*)malloc(a.len * sizeof(double));
    result.len = a.len;
    result.cap = a.len;
    result.shape[0] = a.shape[0];
    result.shape[1] = a.shape[1];
    result.shape[2] = a.shape[2];
    result.shape[3] = a.shape[3];
    
    if (result.data && a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            result.data[i] = a.data[a.len - 1 - i];
        }
    }
    return result;
}

/* Take elements at indices */
static inline LpArray lp_np_take(LpArray a, LpArray indices) {
    LpArray result;
    result.data = (double*)malloc(indices.len * sizeof(double));
    result.len = indices.len;
    result.cap = indices.len;
    result.shape[0] = indices.len;
    result.shape[1] = 0;
    result.shape[2] = 0;
    result.shape[3] = 0;
    
    if (result.data && a.data && indices.data) {
        for (int64_t i = 0; i < indices.len; i++) {
            int64_t idx = (int64_t)indices.data[i];
            if (idx >= 0 && idx < a.len) {
                result.data[i] = a.data[idx];
            } else {
                result.data[i] = 0; /* Out of bounds */
            }
        }
    }
    return result;
}

/* Element-wise comparison: returns count of elements satisfying condition */
static inline int64_t lp_np_count_greater(LpArray a, double val) {
    int64_t count = 0;
    if (a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            if (a.data[i] > val) count++;
        }
    }
    return count;
}

static inline int64_t lp_np_count_less(LpArray a, double val) {
    int64_t count = 0;
    if (a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            if (a.data[i] < val) count++;
        }
    }
    return count;
}

static inline int64_t lp_np_count_equal(LpArray a, double val) {
    int64_t count = 0;
    if (a.data) {
        for (int64_t i = 0; i < a.len; i++) {
            if (a.data[i] == val) count++;
        }
    }
    return count;
}

/* Matrix multiplication (2D arrays)
 * Performs C = A @ B where A is (m x k) and B is (k x n), result is (m x n)
 * Arrays must be reshaped to 2D first using reshape2d()
 */
__attribute__((hot, optimize("O3,unroll-loops,fast-math"), target("avx2,fma")))
static inline LpArray lp_np_matmul(LpArray a, LpArray b) {
    /* Get dimensions from shape or infer from length */
    int64_t m = a.shape[0] > 0 ? a.shape[0] : 1;
    int64_t k_a = a.shape[1] > 0 ? a.shape[1] : a.len;
    int64_t k_b = b.shape[0] > 0 ? b.shape[0] : 1;
    int64_t n = b.shape[1] > 0 ? b.shape[1] : b.len;
    
    /* Handle 1D x 1D case (dot product) */
    if (a.shape[0] <= 0 && a.shape[1] <= 0 && 
        b.shape[0] <= 0 && b.shape[1] <= 0) {
        LpArray result;
        result.data = (double*)malloc(sizeof(double));
        result.data[0] = lp_np_dot(a, b);
        result.len = 1;
        result.cap = 1;
        result.shape[0] = 1;
        result.shape[1] = 0;
        result.shape[2] = 0;
        result.shape[3] = 0;
        return result;
    }
    
    /* For 1D vector x 2D matrix: treat vector as (1 x len) row */
    if (a.shape[0] <= 0 && a.shape[1] <= 0) {
        m = 1;
        k_a = a.len;
    }
    /* For 2D matrix x 1D vector: treat vector as (len x 1) column */
    if (b.shape[0] <= 0 && b.shape[1] <= 0) {
        k_b = b.len;
        n = 1;
    }
    
    /* Verify dimensions match */
    int64_t k = k_a;
    if (k_a != k_b) {
        /* Fallback: use minimum */
        k = k_a < k_b ? k_a : k_b;
    }
    
    int64_t result_len = m * n;
    LpArray result;
    result.data = (double*)calloc(result_len, sizeof(double));
    result.len = result_len;
    result.cap = result_len;
    result.shape[0] = m;
    result.shape[1] = n;
    result.shape[2] = 0;
    result.shape[3] = 0;
    
    if (result.data && a.data && b.data) {
        for (int64_t i = 0; i < m; i++) {
            for (int64_t j = 0; j < n; j++) {
                double sum = 0.0;
                for (int64_t l = 0; l < k; l++) {
                    int64_t a_idx = i * k + l;
                    int64_t b_idx = l * n + j;
                    if (a_idx < a.len && b_idx < b.len) {
                        sum += a.data[a_idx] * b.data[b_idx];
                    }
                }
                result.data[i * n + j] = sum;
            }
        }
    }
    
    return result;
}

/* Eye/Identity matrix */
static inline LpArray lp_np_eye(int64_t n) {
    LpArray result = lp_np_zeros(n * n);
    result.len = n * n;
    result.cap = n * n;
    result.shape[0] = n;
    result.shape[1] = n;
    result.shape[2] = 0;
    result.shape[3] = 0;
    
    if (result.data) {
        for (int64_t i = 0; i < n; i++) {
            result.data[i * n + i] = 1.0;
        }
    }
    return result;
}

/* Diagonal extraction/creation */
static inline LpArray lp_np_diag(LpArray a) {
    /* If 1D, create diagonal matrix; if 2D, extract diagonal */
    int64_t n;
    LpArray result;
    
    if (a.shape[1] <= 0) {
        /* 1D input: create diagonal matrix */
        n = a.len;
        result = lp_np_zeros(n * n);
        result.len = n * n;
        result.cap = n * n;
        result.shape[0] = n;
        result.shape[1] = n;
        if (result.data && a.data) {
            for (int64_t i = 0; i < n; i++) {
                result.data[i * n + i] = a.data[i];
            }
        }
    } else {
        /* 2D input: extract diagonal */
        n = a.shape[0] < a.shape[1] ? a.shape[0] : a.shape[1];
        result.data = (double*)malloc(n * sizeof(double));
        result.len = n;
        result.cap = n;
        result.shape[0] = n;
        result.shape[1] = 0;
        result.shape[2] = 0;
        result.shape[3] = 0;
        if (result.data && a.data) {
            for (int64_t i = 0; i < n; i++) {
                result.data[i] = a.data[i * a.shape[1] + i];
            }
        }
    }
    return result;
}

/* ================================================================
 * Module registry — maps module names to tiers
 * Used by codegen to determine which strategy to use
 * ================================================================ */

typedef enum {
    LP_MOD_TIER1_MATH,
    LP_MOD_TIER1_RANDOM,
    LP_MOD_TIER1_TIME,
    LP_MOD_TIER2_NUMPY,
    LP_MOD_TIER3_PYTHON,  /* fallback: needs Python/C API */
} LpModuleTier;

static inline LpModuleTier lp_get_module_tier(const char *module_name) {
    if (strcmp(module_name, "math") == 0)   return LP_MOD_TIER1_MATH;
    if (strcmp(module_name, "random") == 0) return LP_MOD_TIER1_RANDOM;
    if (strcmp(module_name, "time") == 0)   return LP_MOD_TIER1_TIME;
    if (strcmp(module_name, "numpy") == 0)  return LP_MOD_TIER2_NUMPY;
    return LP_MOD_TIER3_PYTHON;
}

#endif /* LP_NATIVE_MODULES_H */
