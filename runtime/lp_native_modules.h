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
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_np_sum(LpArray a) {
    double s = 0.0;
    for (int64_t i = 0; i < a.len; i++) s += a.data[i];
    return s;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_np_mean(LpArray a) {
    return (a.len > 0) ? lp_np_sum(a) / (double)a.len : 0.0;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_np_min(LpArray a) {
    if (a.len == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.len; i++) if (a.data[i] < m) m = a.data[i];
    return m;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_np_max(LpArray a) {
    if (a.len == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.len; i++) if (a.data[i] > m) m = a.data[i];
    return m;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_np_dot(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    double s = 0.0;
    for (int64_t i = 0; i < n; i++) s += a.data[i] * b.data[i];
    return s;
}

__attribute__((hot, optimize("O3,unroll-loops")))
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
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_add(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] + b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_sub(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] - b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_mul(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] * b.data[i];
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_div(LpArray a, LpArray b) {
    int64_t n = a.len < b.len ? a.len : b.len;
    LpArray r = lp_np_zeros(n);
    for (int64_t i = 0; i < n; i++) r.data[i] = a.data[i] / b.data[i];
    return r;
}

/* Scalar operations */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_scale(LpArray a, double s) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = a.data[i] * s;
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_add_scalar(LpArray a, double s) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = a.data[i] + s;
    return r;
}

/* Math operations on arrays */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_sqrt_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = sqrt(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_abs_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = fabs(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_sin_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = sin(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_cos_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = cos(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_exp_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = exp(a.data[i]);
    return r;
}

__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpArray lp_np_log_arr(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    for (int64_t i = 0; i < a.len; i++) r.data[i] = log(a.data[i]);
    return r;
}

/* Sorting (in-place) */
static int _lp_np_cmp_asc(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

static inline LpArray lp_np_sort(LpArray a) {
    LpArray r = lp_np_zeros(a.len);
    memcpy(r.data, a.data, a.len * sizeof(double));
    qsort(r.data, r.len, sizeof(double), _lp_np_cmp_asc);
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
