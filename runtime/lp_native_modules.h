/*
 * LP Native Module Bindings — Zero-Overhead C Implementations
 *
 * Instead of calling Python at runtime, we map common Python module
 * functions directly to optimized C equivalents. This gives LP programs
 * access to "Python-like" APIs with native C/C++ performance.
 *
 * Strategy:
 *   Tier 1 (ZERO overhead): math, random, os, sys, time — direct C mapping
 *   Tier 2 (Near-zero): tensor/numpy array ops — SIMD-optimized C loops (lp_tensor.h)
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
static inline __attribute__((simd)) double lp_math_sqrt(double x) { return sqrt(x); }
static inline double lp_math_fabs(double x)       { return fabs(x); }
static inline double lp_math_ceil(double x)       { return ceil(x); }
static inline double lp_math_floor(double x)      { return floor(x); }
static inline double lp_math_round(double x)      { return round(x); }
static inline double lp_math_trunc(double x)      { return trunc(x); }

/* Trig */
static inline __attribute__((simd)) double lp_math_sin(double x)  { return sin(x); }
static inline __attribute__((simd)) double lp_math_cos(double x)  { return cos(x); }
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
static inline __attribute__((simd)) double lp_math_log(double x)  { return log(x); }
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
 * TIER 2: Tensor operations — unified numpy + tensor module
 *
 * All numpy/tensor array operations are now in lp_tensor.h.
 * lp_np_* backward compat macros are defined there.
 * lp_tensor.h is included via lp_array.h (included above).
 * ================================================================ */

/* ========================================
 * ADDITIONAL MATH FUNCTIONS (P1)
 * ======================================== */

/* math.comb(n, k) — binomial coefficient */
static inline int64_t lp_math_comb(int64_t n, int64_t k) {
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;
    int64_t result = 1;
    for (int64_t i = 0; i < k; i++) {
        result = result * (n - i) / (i + 1);
    }
    return result;
}

/* math.perm(n, k) — permutations */
static inline int64_t lp_math_perm(int64_t n, int64_t k) {
    if (k < 0 || k > n) return 0;
    int64_t result = 1;
    for (int64_t i = 0; i < k; i++) {
        result *= (n - i);
    }
    return result;
}

/* math.copysign(x, y) */
static inline double lp_math_copysign(double x, double y) {
    return copysign(x, y);
}

/* math.hypot(x, y) */
static inline double lp_math_hypot(double x, double y) {
    return hypot(x, y);
}

/* math.remainder(x, y) — IEEE remainder */
static inline double lp_math_remainder(double x, double y) {
    return remainder(x, y);
}

/* math.erf(x) / math.erfc(x) */
static inline double lp_math_erf(double x) { return erf(x); }
static inline double lp_math_erfc(double x) { return erfc(x); }

/* math.gamma(x) / math.lgamma(x) */
static inline double lp_math_gamma(double x) { return tgamma(x); }
static inline double lp_math_lgamma(double x) { return lgamma(x); }

/* math.prod(list) — product of array elements */
static inline double lp_math_prod_arr(LpTensor a) {
    if (a.size == 0) return 1.0;
    double result = 1.0;
    for (int64_t i = 0; i < a.size; i++) {
        result *= a.data[i];
    }
    return result;
}

/* ========================================
 * ADDITIONAL RANDOM FUNCTIONS (P1)
 * ======================================== */

/* random.choice(list) — random element from LpList */
static inline LpVal lp_random_choice_list(LpList *l) {
    if (!l || !l->items || l->len == 0) return lp_val_null();
    int64_t idx = rand() % l->len;
    return l->items[idx];
}

/* random.shuffle(list) — Fisher-Yates shuffle in-place */
static inline void lp_random_shuffle_list(LpList *l) {
    if (!l || !l->items || l->len <= 1) return;
    for (int64_t i = l->len - 1; i > 0; i--) {
        int64_t j = rand() % (i + 1);
        LpVal tmp = l->items[i];
        l->items[i] = l->items[j];
        l->items[j] = tmp;
    }
}

static inline void lp_random_shuffle_float_array(LpFloatArray *arr) {
    if (!arr || !arr->data || arr->len <= 1) return;
    for (int64_t i = arr->len - 1; i > 0; i--) {
        int64_t j = rand() % (i + 1);
        double tmp = arr->data[i];
        arr->data[i] = arr->data[j];
        arr->data[j] = tmp;
    }
}

static inline void lp_random_shuffle_int_array(LpIntArray *arr) {
    if (!arr || !arr->data || arr->len <= 1) return;
    for (int64_t i = arr->len - 1; i > 0; i--) {
        int64_t j = rand() % (i + 1);
        int64_t tmp = arr->data[i];
        arr->data[i] = arr->data[j];
        arr->data[j] = tmp;
    }
}

/* Macro to dispatch random.shuffle to the correct implementation based on type */
#define lp_random_shuffle(x) _Generic((x), \
    LpList*: lp_random_shuffle_list, \
    LpFloatArray*: lp_random_shuffle_float_array, \
    LpIntArray*: lp_random_shuffle_int_array \
)(x)

/* random.gauss(mu, sigma) — Gaussian/Normal distribution (Box-Muller) */
static inline double lp_random_gauss(double mu, double sigma) {
    static int has_spare = 0;
    static double spare;
    if (has_spare) {
        has_spare = 0;
        return spare * sigma + mu;
    }
    has_spare = 1;
    double u, v, s;
    do {
        u = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        v = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        s = u * u + v * v;
    } while (s >= 1.0 || s == 0.0);
    s = sqrt(-2.0 * log(s) / s);
    spare = v * s;
    return u * s * sigma + mu;
}

/* random.sample(list, k) — random sample without replacement */
static inline LpList* lp_random_sample_list(LpList *l, int64_t k) {
    LpList *result = lp_list_new();
    if (!l || !l->items || l->len == 0 || k <= 0) return result;
    if (k > l->len) k = l->len;
    
    /* Fisher-Yates partial shuffle on a copy of indices */
    int64_t *indices = (int64_t*)malloc(l->len * sizeof(int64_t));
    if (!indices) return result;
    for (int64_t i = 0; i < l->len; i++) indices[i] = i;
    
    for (int64_t i = 0; i < k; i++) {
        int64_t j = i + (rand() % (l->len - i));
        int64_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
        lp_list_append(result, l->items[indices[i]]);
    }
    free(indices);
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
    LP_MOD_TIER2_TENSOR,
    LP_MOD_TIER3_PYTHON,  /* fallback: needs Python/C API */
} LpModuleTier;

static inline LpModuleTier lp_get_module_tier(const char *module_name) {
    if (strcmp(module_name, "math") == 0)   return LP_MOD_TIER1_MATH;
    if (strcmp(module_name, "random") == 0) return LP_MOD_TIER1_RANDOM;
    if (strcmp(module_name, "time") == 0)   return LP_MOD_TIER1_TIME;
    if (strcmp(module_name, "numpy") == 0)  return LP_MOD_TIER2_NUMPY;
    if (strcmp(module_name, "tensor") == 0) return LP_MOD_TIER2_TENSOR;
    return LP_MOD_TIER3_PYTHON;
}

#endif /* LP_NATIVE_MODULES_H */
