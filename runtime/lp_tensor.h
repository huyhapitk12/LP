/*
 * LP Tensor Library — N-Dimensional Tensor with SIMD Optimization
 *
 * Unified replacement for the old numpy/LpArray system.
 * Supports arbitrary dimensions (up to 8), strides, views,
 * broadcasting, and axis-based reductions.
 *
 * Memory layout: Row-major (C-contiguous) by default.
 * All hot loops use GCC auto-vectorization attributes.
 */

#ifndef LP_TENSOR_H
#define LP_TENSOR_H

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
  /* Note: Do NOT include <windows.h> here — it conflicts with LP compiler
   * symbols (TokenType, NODE_ATTRIBUTE). lp_tensor.h doesn't need Win32 API. */
#else
  #include <unistd.h>
#endif

/* ================================================================
 * Core Data Structure
 * ================================================================ */

#define LP_TENSOR_MAX_NDIM 8

typedef struct {
    double  *data;                          /* Contiguous data buffer */
    int64_t  ndim;                          /* Number of dimensions (rank) */
    int64_t  shape[LP_TENSOR_MAX_NDIM];     /* Size of each dimension */
    int64_t  strides[LP_TENSOR_MAX_NDIM];   /* Stride for each dim (in elements) */
    int64_t  size;                          /* Total number of elements */
    int      owns_data;                     /* 1 = owns data (free on destroy) */
} LpTensor;

/* ================================================================
 * Internal Helpers
 * ================================================================ */

/* Compute row-major strides from shape */
static inline void _lp_tensor_compute_strides(LpTensor *t) {
    if (t->ndim == 0) return;
    t->strides[t->ndim - 1] = 1;
    for (int64_t i = t->ndim - 2; i >= 0; i--) {
        t->strides[i] = t->strides[i + 1] * t->shape[i + 1];
    }
}

/* Compute total size from shape */
static inline int64_t _lp_tensor_compute_size(const int64_t *shape, int64_t ndim) {
    if (ndim == 0) return 1;
    int64_t s = 1;
    for (int64_t i = 0; i < ndim; i++) s *= shape[i];
    return s;
}

/* Convert flat index to multi-dimensional index */
static inline void _lp_tensor_unravel(int64_t flat, const int64_t *shape,
                                       int64_t ndim, int64_t *out_idx) {
    for (int64_t i = ndim - 1; i >= 0; i--) {
        out_idx[i] = flat % shape[i];
        flat /= shape[i];
    }
}

/* Convert multi-dimensional index to flat offset using strides */
static inline int64_t _lp_tensor_ravel(const int64_t *idx,
                                        const int64_t *strides, int64_t ndim) {
    int64_t off = 0;
    for (int64_t i = 0; i < ndim; i++) off += idx[i] * strides[i];
    return off;
}

/* Check if tensor is contiguous (row-major) */
static inline int _lp_tensor_is_contiguous(const LpTensor *t) {
    if (t->ndim == 0) return 1;
    int64_t expected = 1;
    for (int64_t i = t->ndim - 1; i >= 0; i--) {
        if (t->strides[i] != expected) return 0;
        expected *= t->shape[i];
    }
    return 1;
}

/* Create tensor with given shape (uninitialized data) */
static inline LpTensor _lp_tensor_alloc(const int64_t *shape, int64_t ndim) {
    LpTensor t;
    t.ndim = ndim;
    t.size = _lp_tensor_compute_size(shape, ndim);
    for (int64_t i = 0; i < ndim; i++) t.shape[i] = shape[i];
    for (int64_t i = ndim; i < LP_TENSOR_MAX_NDIM; i++) t.shape[i] = 0;
    _lp_tensor_compute_strides(&t);
    t.data = (double *)malloc(t.size * sizeof(double));
    t.owns_data = 1;
    return t;
}

/* ================================================================
 * Broadcasting Engine
 * ================================================================ */

/* Compute broadcast shape. Returns 0 on success, -1 on incompatible. */
static inline int _lp_tensor_broadcast_shape(
    const int64_t *sa, int64_t na,
    const int64_t *sb, int64_t nb,
    int64_t *out_shape, int64_t *out_ndim)
{
    int64_t nd = na > nb ? na : nb;
    *out_ndim = nd;
    for (int64_t i = 0; i < nd; i++) {
        int64_t da = (i < na) ? sa[na - 1 - i] : 1;
        int64_t db = (i < nb) ? sb[nb - 1 - i] : 1;
        if (da == db) {
            out_shape[nd - 1 - i] = da;
        } else if (da == 1) {
            out_shape[nd - 1 - i] = db;
        } else if (db == 1) {
            out_shape[nd - 1 - i] = da;
        } else {
            return -1; /* Incompatible shapes */
        }
    }
    return 0;
}

/* Map a flat index in the broadcast result back to flat index in source tensor */
static inline int64_t _lp_tensor_broadcast_index(
    int64_t flat_result,
    const int64_t *result_shape, int64_t result_ndim,
    const int64_t *src_shape, const int64_t *src_strides, int64_t src_ndim)
{
    int64_t idx[LP_TENSOR_MAX_NDIM];
    _lp_tensor_unravel(flat_result, result_shape, result_ndim, idx);

    int64_t offset = 0;
    for (int64_t i = 0; i < result_ndim; i++) {
        int64_t si = i - (result_ndim - src_ndim);
        if (si >= 0) {
            int64_t src_idx = (src_shape[si] == 1) ? 0 : idx[i];
            offset += src_idx * src_strides[si];
        }
    }
    return offset;
}

/* ================================================================
 * CREATION FUNCTIONS
 * ================================================================ */

/* tensor.zeros(shape) — shape passed as array of dims */
static inline LpTensor lp_tensor_zeros_nd(const int64_t *shape, int64_t ndim) {
    LpTensor t = _lp_tensor_alloc(shape, ndim);
    if (t.data) memset(t.data, 0, t.size * sizeof(double));
    return t;
}

/* tensor.zeros(n) — 1D convenience */
static inline LpTensor lp_tensor_zeros_1d(int64_t n) {
    int64_t shape[1] = {n};
    return lp_tensor_zeros_nd(shape, 1);
}

/* tensor.ones(shape) */
static inline LpTensor lp_tensor_ones_nd(const int64_t *shape, int64_t ndim) {
    LpTensor t = _lp_tensor_alloc(shape, ndim);
    if (t.data) {
        for (int64_t i = 0; i < t.size; i++) t.data[i] = 1.0;
    }
    return t;
}

static inline LpTensor lp_tensor_ones_1d(int64_t n) {
    int64_t shape[1] = {n};
    return lp_tensor_ones_nd(shape, 1);
}

/* tensor.full(shape, value) */
static inline LpTensor lp_tensor_full_nd(const int64_t *shape, int64_t ndim, double val) {
    LpTensor t = _lp_tensor_alloc(shape, ndim);
    if (t.data) {
        for (int64_t i = 0; i < t.size; i++) t.data[i] = val;
    }
    return t;
}

/* tensor.arange(start, stop, step) */
static inline LpTensor lp_tensor_arange(double start, double stop, double step) {
    int64_t n = (int64_t)((stop - start) / step);
    if (n < 0) n = 0;
    int64_t shape[1] = {n};
    LpTensor t = _lp_tensor_alloc(shape, 1);
    if (t.data) {
        for (int64_t i = 0; i < n; i++) t.data[i] = start + i * step;
    }
    return t;
}

/* tensor.linspace(start, stop, n) */
static inline LpTensor lp_tensor_linspace(double start, double stop, int64_t n) {
    int64_t shape[1] = {n};
    LpTensor t = _lp_tensor_alloc(shape, 1);
    if (t.data) {
        double step = (n > 1) ? (stop - start) / (n - 1) : 0.0;
        for (int64_t i = 0; i < n; i++) t.data[i] = start + i * step;
    }
    return t;
}

/* tensor.eye(n) — identity matrix */
static inline LpTensor lp_tensor_eye(int64_t n) {
    int64_t shape[2] = {n, n};
    LpTensor t = lp_tensor_zeros_nd(shape, 2);
    if (t.data) {
        for (int64_t i = 0; i < n; i++) t.data[i * n + i] = 1.0;
    }
    return t;
}

/* tensor.rand(shape) — uniform [0, 1) */
static inline LpTensor lp_tensor_rand_nd(const int64_t *shape, int64_t ndim) {
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
    LpTensor t = _lp_tensor_alloc(shape, ndim);
    if (t.data) {
        for (int64_t i = 0; i < t.size; i++)
            t.data[i] = (double)rand() / (double)RAND_MAX;
    }
    return t;
}

/* tensor.randn(shape) — normal distribution (Box-Muller) */
static inline LpTensor lp_tensor_randn_nd(const int64_t *shape, int64_t ndim) {
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
    LpTensor t = _lp_tensor_alloc(shape, ndim);
    if (t.data) {
        for (int64_t i = 0; i < t.size; i += 2) {
            double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
            double u2 = (double)rand() / (double)RAND_MAX;
            double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * 3.14159265358979323846 * u2);
            double z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * 3.14159265358979323846 * u2);
            t.data[i] = z0;
            if (i + 1 < t.size) t.data[i + 1] = z1;
        }
    }
    return t;
}

/* tensor.from_doubles(count, ...) — used by codegen for tensor.from_list([1,2,3]) */
static inline LpTensor lp_tensor_from_doubles(int64_t count, ...) {
    int64_t shape[1] = {count};
    LpTensor t = _lp_tensor_alloc(shape, 1);
    if (t.data) {
        va_list ap;
        va_start(ap, count);
        for (int64_t i = 0; i < count; i++)
            t.data[i] = va_arg(ap, double);
        va_end(ap);
    }
    return t;
}

/* ================================================================
 * SHAPE MANIPULATION
 * ================================================================ */

/* tensor.reshape(t, new_shape) — returns view if contiguous, copy otherwise */
static inline LpTensor lp_tensor_reshape(LpTensor a, const int64_t *new_shape, int64_t new_ndim) {
    LpTensor t;
    t.ndim = new_ndim;
    t.size = a.size;
    for (int64_t i = 0; i < new_ndim; i++) t.shape[i] = new_shape[i];
    for (int64_t i = new_ndim; i < LP_TENSOR_MAX_NDIM; i++) t.shape[i] = 0;
    _lp_tensor_compute_strides(&t);

    if (_lp_tensor_is_contiguous(&a)) {
        /* View — share data */
        t.data = a.data;
        t.owns_data = 0;
    } else {
        /* Must copy to contiguous first */
        t.data = (double *)malloc(t.size * sizeof(double));
        t.owns_data = 1;
        if (t.data) {
            for (int64_t i = 0; i < a.size; i++) {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                t.data[i] = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];
            }
        }
    }
    return t;
}

/* Codegen helper: reshape with 2 dims (backward compat with np.reshape(arr, rows, cols)) */
static inline LpTensor lp_tensor_reshape2d(LpTensor a, int64_t rows, int64_t cols) {
    int64_t shape[2] = {rows, cols};
    return lp_tensor_reshape(a, shape, 2);
}

/* tensor.transpose(t, dim0, dim1) — swap two dimensions */
static inline LpTensor lp_tensor_transpose(LpTensor a, int64_t dim0, int64_t dim1) {
    LpTensor t;
    t.ndim = a.ndim;
    t.size = a.size;
    t.owns_data = 0; /* view */
    t.data = a.data;
    for (int64_t i = 0; i < a.ndim; i++) {
        t.shape[i] = a.shape[i];
        t.strides[i] = a.strides[i];
    }
    for (int64_t i = a.ndim; i < LP_TENSOR_MAX_NDIM; i++) {
        t.shape[i] = 0;
        t.strides[i] = 0;
    }
    /* Swap */
    int64_t tmp;
    tmp = t.shape[dim0]; t.shape[dim0] = t.shape[dim1]; t.shape[dim1] = tmp;
    tmp = t.strides[dim0]; t.strides[dim0] = t.strides[dim1]; t.strides[dim1] = tmp;
    return t;
}

/* Backward compat: transpose for 2D (no args, swap 0,1) */
static inline LpTensor lp_tensor_transpose_2d(LpTensor a) {
    if (a.ndim < 2) return a;
    /* For non-contiguous, we need to produce contiguous output for safety */
    int64_t rows = a.shape[0];
    int64_t cols = a.shape[1] > 0 ? a.shape[1] : a.size;
    int64_t shape[2] = {cols, rows};
    LpTensor t = _lp_tensor_alloc(shape, 2);
    if (t.data && a.data) {
        for (int64_t i = 0; i < rows; i++) {
            for (int64_t j = 0; j < cols; j++) {
                int64_t src_idx[LP_TENSOR_MAX_NDIM] = {i, j};
                int64_t dst_idx[LP_TENSOR_MAX_NDIM] = {j, i};
                t.data[_lp_tensor_ravel(dst_idx, t.strides, 2)] =
                    a.data[_lp_tensor_ravel(src_idx, a.strides, a.ndim)];
            }
        }
    }
    return t;
}

/* tensor.permute(t, dims...) — general permutation */
static inline LpTensor lp_tensor_permute(LpTensor a, const int64_t *perm, int64_t ndim) {
    LpTensor t;
    t.ndim = ndim;
    t.size = a.size;
    t.data = a.data;
    t.owns_data = 0;
    for (int64_t i = 0; i < ndim; i++) {
        t.shape[i] = a.shape[perm[i]];
        t.strides[i] = a.strides[perm[i]];
    }
    for (int64_t i = ndim; i < LP_TENSOR_MAX_NDIM; i++) {
        t.shape[i] = 0; t.strides[i] = 0;
    }
    return t;
}

/* tensor.squeeze(t) — remove all dims of size 1 */
static inline LpTensor lp_tensor_squeeze(LpTensor a) {
    LpTensor t;
    t.size = a.size;
    t.data = a.data;
    t.owns_data = 0;
    t.ndim = 0;
    for (int64_t i = 0; i < a.ndim; i++) {
        if (a.shape[i] != 1) {
            t.shape[t.ndim] = a.shape[i];
            t.strides[t.ndim] = a.strides[i];
            t.ndim++;
        }
    }
    if (t.ndim == 0) { t.ndim = 1; t.shape[0] = 1; t.strides[0] = 1; }
    for (int64_t i = t.ndim; i < LP_TENSOR_MAX_NDIM; i++) {
        t.shape[i] = 0; t.strides[i] = 0;
    }
    return t;
}

/* tensor.unsqueeze(t, dim) — insert dim of size 1 */
static inline LpTensor lp_tensor_unsqueeze(LpTensor a, int64_t dim) {
    if (a.ndim >= LP_TENSOR_MAX_NDIM) return a;
    LpTensor t;
    t.size = a.size;
    t.data = a.data;
    t.owns_data = 0;
    t.ndim = a.ndim + 1;
    for (int64_t i = 0; i < dim; i++) {
        t.shape[i] = a.shape[i];
        t.strides[i] = a.strides[i];
    }
    t.shape[dim] = 1;
    t.strides[dim] = (dim < a.ndim) ? a.strides[dim] * a.shape[dim] : 1;
    for (int64_t i = dim; i < a.ndim; i++) {
        t.shape[i + 1] = a.shape[i];
        t.strides[i + 1] = a.strides[i];
    }
    for (int64_t i = t.ndim; i < LP_TENSOR_MAX_NDIM; i++) {
        t.shape[i] = 0; t.strides[i] = 0;
    }
    return t;
}

/* tensor.flatten(t) — to 1D, always copies */
static inline LpTensor lp_tensor_flatten(LpTensor a) {
    int64_t shape[1] = {a.size};
    LpTensor t = _lp_tensor_alloc(shape, 1);
    if (t.data && a.data) {
        if (_lp_tensor_is_contiguous(&a)) {
            memcpy(t.data, a.data, a.size * sizeof(double));
        } else {
            for (int64_t i = 0; i < a.size; i++) {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                t.data[i] = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];
            }
        }
    }
    return t;
}

/* tensor.contiguous(t) — return contiguous copy if not already */
static inline LpTensor lp_tensor_contiguous(LpTensor a) {
    if (_lp_tensor_is_contiguous(&a)) {
        LpTensor t = a;
        t.owns_data = 0;
        return t;
    }
    return lp_tensor_flatten(a); /* flatten always produces contiguous */
}

/* ================================================================
 * ELEMENT ACCESS
 * ================================================================ */

/* tensor.get(t, i0, i1, ...) — get element (variadic) */
static inline double lp_tensor_get_v(LpTensor t, int64_t nidx, ...) {
    int64_t idx[LP_TENSOR_MAX_NDIM] = {0};
    va_list ap;
    va_start(ap, nidx);
    for (int64_t i = 0; i < nidx && i < t.ndim; i++)
        idx[i] = va_arg(ap, int64_t);
    va_end(ap);
    return t.data[_lp_tensor_ravel(idx, t.strides, t.ndim)];
}

/* tensor.set(t, value, i0, i1, ...) */
static inline void lp_tensor_set_v(LpTensor *t, double val, int64_t nidx, ...) {
    int64_t idx[LP_TENSOR_MAX_NDIM] = {0};
    va_list ap;
    va_start(ap, nidx);
    for (int64_t i = 0; i < nidx && i < t->ndim; i++)
        idx[i] = va_arg(ap, int64_t);
    va_end(ap);
    t->data[_lp_tensor_ravel(idx, t->strides, t->ndim)] = val;
}

/* ================================================================
 * ELEMENT-WISE OPERATIONS (with broadcasting)
 * All functions: hot, O3, auto-vectorized
 * ================================================================ */

#define LP_TENSOR_BINOP(name, op)                                              \
__attribute__((hot, optimize("O3,unroll-loops")))                               \
static inline LpTensor lp_tensor_##name(LpTensor a, LpTensor b) {             \
    int64_t out_shape[LP_TENSOR_MAX_NDIM];                                     \
    int64_t out_ndim;                                                          \
    if (_lp_tensor_broadcast_shape(a.shape, a.ndim, b.shape, b.ndim,           \
                                    out_shape, &out_ndim) != 0) {              \
        /* Incompatible — return empty */                                      \
        return lp_tensor_zeros_1d(0);                                          \
    }                                                                          \
    LpTensor r = _lp_tensor_alloc(out_shape, out_ndim);                        \
    if (r.data && a.data && b.data) {                                          \
        /* Fast path: same shape, both contiguous */                            \
        if (a.ndim == b.ndim && a.size == b.size && a.size == r.size &&         \
            _lp_tensor_is_contiguous(&a) && _lp_tensor_is_contiguous(&b)) {    \
            for (int64_t i = 0; i < r.size; i++)                               \
                r.data[i] = a.data[i] op b.data[i];                           \
        } else {                                                               \
            for (int64_t i = 0; i < r.size; i++) {                             \
                int64_t ai = _lp_tensor_broadcast_index(i, out_shape, out_ndim,\
                    a.shape, a.strides, a.ndim);                               \
                int64_t bi = _lp_tensor_broadcast_index(i, out_shape, out_ndim,\
                    b.shape, b.strides, b.ndim);                               \
                r.data[i] = a.data[ai] op b.data[bi];                         \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    return r;                                                                  \
}

LP_TENSOR_BINOP(add, +)
LP_TENSOR_BINOP(sub, -)
LP_TENSOR_BINOP(mul, *)
LP_TENSOR_BINOP(div, /)

#undef LP_TENSOR_BINOP

/* Scalar operations */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpTensor lp_tensor_scale(LpTensor a, double s) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        if (_lp_tensor_is_contiguous(&a)) {
            for (int64_t i = 0; i < r.size; i++) r.data[i] = a.data[i] * s;
        } else {
            for (int64_t i = 0; i < r.size; i++) {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                r.data[i] = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)] * s;
            }
        }
    }
    return r;
}

static inline LpTensor lp_tensor_add_scalar(LpTensor a, double s) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        for (int64_t i = 0; i < r.size; i++) r.data[i] = a.data[i] + s;
    }
    return r;
}

/* Negation */
static inline LpTensor lp_tensor_neg(LpTensor a) {
    return lp_tensor_scale(a, -1.0);
}

/* ================================================================
 * UNARY MATH OPERATIONS
 * ================================================================ */

#define LP_TENSOR_UNARY(name, expr)                                            \
__attribute__((hot, optimize("O3,unroll-loops")))                               \
static inline LpTensor lp_tensor_##name(LpTensor a) {                         \
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);                           \
    if (r.data && a.data) {                                                    \
        if (_lp_tensor_is_contiguous(&a)) {                                    \
            for (int64_t i = 0; i < r.size; i++) {                             \
                double x = a.data[i]; (void)x;                                \
                r.data[i] = (expr);                                            \
            }                                                                  \
        } else {                                                               \
            for (int64_t i = 0; i < r.size; i++) {                             \
                int64_t idx[LP_TENSOR_MAX_NDIM];                               \
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);                   \
                double x = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];   \
                (void)x;                                                       \
                r.data[i] = (expr);                                            \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    return r;                                                                  \
}

LP_TENSOR_UNARY(abs_t,    fabs(x))
LP_TENSOR_UNARY(sqrt_t,   sqrt(x))
LP_TENSOR_UNARY(exp_t,    exp(x))
LP_TENSOR_UNARY(log_t,    log(x))
LP_TENSOR_UNARY(sin_t,    sin(x))
LP_TENSOR_UNARY(cos_t,    cos(x))
LP_TENSOR_UNARY(tanh_t,   tanh(x))
LP_TENSOR_UNARY(sigmoid,  1.0 / (1.0 + exp(-x)))
LP_TENSOR_UNARY(relu,     x > 0.0 ? x : 0.0)

#undef LP_TENSOR_UNARY

/* tensor.pow(t, p) — element-wise power */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpTensor lp_tensor_pow_t(LpTensor a, double p) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        for (int64_t i = 0; i < r.size; i++)
            r.data[i] = pow(a.data[i], p);
    }
    return r;
}

/* tensor.clamp(t, min, max) */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpTensor lp_tensor_clamp(LpTensor a, double lo, double hi) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        for (int64_t i = 0; i < r.size; i++) {
            double v = a.data[i];
            r.data[i] = v < lo ? lo : (v > hi ? hi : v);
        }
    }
    return r;
}

/* ================================================================
 * REDUCTION OPERATIONS
 * axis = -1 means reduce ALL dimensions (return scalar-shaped tensor)
 * ================================================================ */

/* Helper: reduce along a specific axis */
static inline LpTensor _lp_tensor_reduce_axis(
    LpTensor a, int64_t axis,
    double init, double (*combine)(double acc, double val), double (*finalize)(double acc, int64_t count))
{
    if (axis < 0 || axis >= a.ndim) {
        /* Reduce all — return 1-element tensor */
        int64_t shape[1] = {1};
        LpTensor r = _lp_tensor_alloc(shape, 1);
        double acc = init;
        for (int64_t i = 0; i < a.size; i++) {
            double v;
            if (_lp_tensor_is_contiguous(&a)) {
                v = a.data[i];
            } else {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                v = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];
            }
            acc = combine(acc, v);
        }
        r.data[0] = finalize ? finalize(acc, a.size) : acc;
        return r;
    }

    /* Build output shape: remove the axis dimension */
    int64_t out_shape[LP_TENSOR_MAX_NDIM];
    int64_t out_ndim = a.ndim - 1;
    if (out_ndim == 0) out_ndim = 1;
    int j = 0;
    for (int64_t i = 0; i < a.ndim; i++) {
        if (i != axis) out_shape[j++] = a.shape[i];
    }
    if (j == 0) { out_shape[0] = 1; j = 1; }

    LpTensor r = lp_tensor_zeros_nd(out_shape, j);

    /* Iterate over all output positions */
    int64_t axis_size = a.shape[axis];
    for (int64_t oi = 0; oi < r.size; oi++) {
        int64_t out_idx[LP_TENSOR_MAX_NDIM];
        _lp_tensor_unravel(oi, out_shape, j, out_idx);

        /* Build source index template */
        int64_t src_idx[LP_TENSOR_MAX_NDIM];
        int k = 0;
        for (int64_t d = 0; d < a.ndim; d++) {
            if (d == axis) {
                src_idx[d] = 0; /* will iterate */
            } else {
                src_idx[d] = out_idx[k++];
            }
        }

        double acc = init;
        for (int64_t ai = 0; ai < axis_size; ai++) {
            src_idx[axis] = ai;
            double v = a.data[_lp_tensor_ravel(src_idx, a.strides, a.ndim)];
            acc = combine(acc, v);
        }
        r.data[oi] = finalize ? finalize(acc, axis_size) : acc;
    }
    return r;
}

/* Combine functions */
static inline double _lp_combine_sum(double a, double b) { return a + b; }
static inline double _lp_combine_max(double a, double b) { return b > a ? b : a; }
static inline double _lp_combine_min(double a, double b) { return b < a ? b : a; }
static inline double _lp_finalize_mean(double a, int64_t n) { return a / (double)n; }

/* tensor.sum — reduce all */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_sum_all(LpTensor a) {
    double s = 0.0;
    if (a.data) {
        if (_lp_tensor_is_contiguous(&a)) {
            for (int64_t i = 0; i < a.size; i++) s += a.data[i];
        } else {
            for (int64_t i = 0; i < a.size; i++) {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                s += a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];
            }
        }
    }
    return s;
}

/* tensor.sum(t, axis) */
static inline LpTensor lp_tensor_sum_axis(LpTensor a, int64_t axis) {
    return _lp_tensor_reduce_axis(a, axis, 0.0, _lp_combine_sum, NULL);
}

/* tensor.mean — reduce all */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_mean_all(LpTensor a) {
    return (a.size > 0) ? lp_tensor_sum_all(a) / (double)a.size : 0.0;
}

/* tensor.mean(t, axis) */
static inline LpTensor lp_tensor_mean_axis(LpTensor a, int64_t axis) {
    return _lp_tensor_reduce_axis(a, axis, 0.0, _lp_combine_sum, _lp_finalize_mean);
}

/* tensor.max — reduce all */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_max_all(LpTensor a) {
    if (!a.data || a.size == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.size; i++) if (a.data[i] > m) m = a.data[i];
    return m;
}

/* tensor.max(t, axis) */
static inline LpTensor lp_tensor_max_axis(LpTensor a, int64_t axis) {
    return _lp_tensor_reduce_axis(a, axis, -1e308, _lp_combine_max, NULL);
}

/* tensor.min — reduce all */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_min_all(LpTensor a) {
    if (!a.data || a.size == 0) return 0.0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.size; i++) if (a.data[i] < m) m = a.data[i];
    return m;
}

/* tensor.min(t, axis) */
static inline LpTensor lp_tensor_min_axis(LpTensor a, int64_t axis) {
    return _lp_tensor_reduce_axis(a, axis, 1e308, _lp_combine_min, NULL);
}

/* tensor.std — all */
static inline double lp_tensor_std_all(LpTensor a) {
    if (a.size <= 1) return 0.0;
    double m = lp_tensor_mean_all(a);
    double s = 0.0;
    for (int64_t i = 0; i < a.size; i++) {
        double d = a.data[i] - m;
        s += d * d;
    }
    return sqrt(s / (double)a.size);
}

/* tensor.var — all */
static inline double lp_tensor_var_all(LpTensor a) {
    double m = lp_tensor_mean_all(a);
    double s = 0.0;
    for (int64_t i = 0; i < a.size; i++) {
        double d = a.data[i] - m;
        s += d * d;
    }
    return s / (double)a.size;
}

/* tensor.median — all */
static inline double lp_tensor_median_all(LpTensor a);  /* forward decl */

/* tensor.norm — L2 norm, all */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_norm_all(LpTensor a) {
    double s = 0.0;
    for (int64_t i = 0; i < a.size; i++) s += a.data[i] * a.data[i];
    return sqrt(s);
}

/* tensor.argmax — all */
static inline int64_t lp_tensor_argmax_all(LpTensor a) {
    if (!a.data || a.size == 0) return -1;
    int64_t idx = 0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.size; i++) {
        if (a.data[i] > m) { m = a.data[i]; idx = i; }
    }
    return idx;
}

/* tensor.argmin — all */
static inline int64_t lp_tensor_argmin_all(LpTensor a) {
    if (!a.data || a.size == 0) return -1;
    int64_t idx = 0;
    double m = a.data[0];
    for (int64_t i = 1; i < a.size; i++) {
        if (a.data[i] < m) { m = a.data[i]; idx = i; }
    }
    return idx;
}

/* ================================================================
 * LINEAR ALGEBRA
 * ================================================================ */

/* tensor.dot(a, b) — 1D dot product */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline double lp_tensor_dot(LpTensor a, LpTensor b) {
    int64_t n = a.size < b.size ? a.size : b.size;
    double s = 0.0;
    for (int64_t i = 0; i < n; i++) s += a.data[i] * b.data[i];
    return s;
}

/* tensor.matmul(a, b) — general matrix multiply
 * Supports: (m,k) x (k,n) -> (m,n)
 *           (b,m,k) x (b,k,n) -> (b,m,n) (batched)
 *           1D x 1D -> scalar (dot product)
 */
__attribute__((hot, optimize("O3,unroll-loops")))
static inline LpTensor lp_tensor_matmul(LpTensor a, LpTensor b) {
    /* 1D x 1D: dot product -> scalar tensor */
    if (a.ndim == 1 && b.ndim == 1) {
        int64_t shape[1] = {1};
        LpTensor r = _lp_tensor_alloc(shape, 1);
        r.data[0] = lp_tensor_dot(a, b);
        return r;
    }

    /* 2D x 2D: standard matmul */
    if (a.ndim == 2 && b.ndim == 2) {
        int64_t m = a.shape[0], k = a.shape[1], n = b.shape[1];
        int64_t shape[2] = {m, n};
        LpTensor r = lp_tensor_zeros_nd(shape, 2);
        if (r.data && a.data && b.data) {
            for (int64_t i = 0; i < m; i++) {
                for (int64_t l = 0; l < k; l++) {
                    double a_il = a.data[i * a.strides[0] + l * a.strides[1]];
                    for (int64_t j = 0; j < n; j++) {
                        r.data[i * n + j] += a_il * b.data[l * b.strides[0] + j * b.strides[1]];
                    }
                }
            }
        }
        return r;
    }

    /* 2D x 1D: matrix-vector -> 1D */
    if (a.ndim == 2 && b.ndim == 1) {
        int64_t m = a.shape[0], k = a.shape[1];
        int64_t shape[1] = {m};
        LpTensor r = lp_tensor_zeros_nd(shape, 1);
        if (r.data && a.data && b.data) {
            for (int64_t i = 0; i < m; i++) {
                double s = 0.0;
                for (int64_t l = 0; l < k; l++)
                    s += a.data[i * a.strides[0] + l * a.strides[1]] * b.data[l];
                r.data[i] = s;
            }
        }
        return r;
    }

    /* 1D x 2D: vector-matrix -> 1D */
    if (a.ndim == 1 && b.ndim == 2) {
        int64_t k = b.shape[0], n = b.shape[1];
        int64_t shape[1] = {n};
        LpTensor r = lp_tensor_zeros_nd(shape, 1);
        if (r.data && a.data && b.data) {
            for (int64_t j = 0; j < n; j++) {
                double s = 0.0;
                for (int64_t l = 0; l < k; l++)
                    s += a.data[l] * b.data[l * b.strides[0] + j * b.strides[1]];
                r.data[j] = s;
            }
        }
        return r;
    }

    /* 3D batched matmul: (batch, m, k) x (batch, k, n) -> (batch, m, n) */
    if (a.ndim == 3 && b.ndim == 3) {
        int64_t batch = a.shape[0], m = a.shape[1], k = a.shape[2], n = b.shape[2];
        int64_t shape[3] = {batch, m, n};
        LpTensor r = lp_tensor_zeros_nd(shape, 3);
        if (r.data && a.data && b.data) {
            for (int64_t bi = 0; bi < batch; bi++) {
                for (int64_t i = 0; i < m; i++) {
                    for (int64_t l = 0; l < k; l++) {
                        double a_val = a.data[bi * a.strides[0] + i * a.strides[1] + l * a.strides[2]];
                        for (int64_t j = 0; j < n; j++) {
                            r.data[bi * m * n + i * n + j] +=
                                a_val * b.data[bi * b.strides[0] + l * b.strides[1] + j * b.strides[2]];
                        }
                    }
                }
            }
        }
        return r;
    }

    /* Fallback: treat as 2D */
    int64_t m = (a.ndim >= 2) ? a.shape[a.ndim - 2] : 1;
    int64_t k = a.shape[a.ndim - 1];
    int64_t n = b.shape[b.ndim - 1];
    int64_t shape[2] = {m, n};
    LpTensor r = lp_tensor_zeros_nd(shape, 2);
    if (r.data && a.data && b.data) {
        for (int64_t i = 0; i < m; i++) {
            for (int64_t l = 0; l < k; l++) {
                double a_val = a.data[i * k + l];
                for (int64_t j = 0; j < n; j++)
                    r.data[i * n + j] += a_val * b.data[l * n + j];
            }
        }
    }
    return r;
}

/* tensor.trace — sum of diagonal elements */
static inline double lp_tensor_trace(LpTensor a) {
    if (a.ndim < 2) return (a.size > 0) ? a.data[0] : 0.0;
    int64_t n = a.shape[0] < a.shape[1] ? a.shape[0] : a.shape[1];
    double s = 0.0;
    for (int64_t i = 0; i < n; i++)
        s += a.data[i * a.strides[0] + i * a.strides[1]];
    return s;
}

/* tensor.diag — extract diagonal or create diagonal matrix */
static inline LpTensor lp_tensor_diag(LpTensor a) {
    if (a.ndim == 1) {
        /* 1D → diagonal matrix */
        int64_t n = a.size;
        int64_t shape[2] = {n, n};
        LpTensor r = lp_tensor_zeros_nd(shape, 2);
        for (int64_t i = 0; i < n; i++)
            r.data[i * n + i] = a.data[i];
        return r;
    } else {
        /* 2D → extract diagonal */
        int64_t n = a.shape[0] < a.shape[1] ? a.shape[0] : a.shape[1];
        int64_t shape[1] = {n};
        LpTensor r = _lp_tensor_alloc(shape, 1);
        for (int64_t i = 0; i < n; i++)
            r.data[i] = a.data[i * a.strides[0] + i * a.strides[1]];
        return r;
    }
}

/* ================================================================
 * ARRAY OPERATIONS (from old numpy)
 * ================================================================ */

/* Sort comparison */
static int _lp_tensor_cmp_asc(const void *a, const void *b) {
    double da = *(const double *)a, db = *(const double *)b;
    return (da > db) - (da < db);
}

/* tensor.sort(t) — returns sorted copy (1D) */
static inline LpTensor lp_tensor_sort(LpTensor a) {
    LpTensor r = lp_tensor_flatten(a);
    if (r.data && r.size > 1)
        qsort(r.data, r.size, sizeof(double), _lp_tensor_cmp_asc);
    return r;
}

/* tensor.reverse(t) */
static inline LpTensor lp_tensor_reverse(LpTensor a) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        for (int64_t i = 0; i < a.size; i++)
            r.data[i] = a.data[a.size - 1 - i];
    }
    return r;
}

/* tensor.cumsum(t) — 1D cumulative sum */
static inline LpTensor lp_tensor_cumsum(LpTensor a) {
    LpTensor flat = lp_tensor_flatten(a);
    LpTensor r = _lp_tensor_alloc(flat.shape, 1);
    if (r.data && flat.data) {
        double s = 0;
        for (int64_t i = 0; i < flat.size; i++) {
            s += flat.data[i];
            r.data[i] = s;
        }
    }
    if (flat.owns_data) free(flat.data);
    return r;
}

/* tensor.diff(t) — consecutive differences */
static inline LpTensor lp_tensor_diff(LpTensor a) {
    if (a.size <= 1) return lp_tensor_zeros_1d(0);
    int64_t shape[1] = {a.size - 1};
    LpTensor r = _lp_tensor_alloc(shape, 1);
    if (r.data && a.data) {
        for (int64_t i = 0; i < r.size; i++)
            r.data[i] = a.data[i + 1] - a.data[i];
    }
    return r;
}

/* tensor.concatenate(a, b) — along first axis (flattened) */
static inline LpTensor lp_tensor_concatenate(LpTensor a, LpTensor b) {
    int64_t shape[1] = {a.size + b.size};
    LpTensor r = _lp_tensor_alloc(shape, 1);
    if (r.data) {
        if (a.data) memcpy(r.data, a.data, a.size * sizeof(double));
        if (b.data) memcpy(r.data + a.size, b.data, b.size * sizeof(double));
    }
    return r;
}

/* tensor.unique(t) */
static inline LpTensor lp_tensor_unique(LpTensor a) {
    if (a.size == 0) return lp_tensor_zeros_1d(0);
    LpTensor sorted = lp_tensor_sort(a);
    /* Count unique */
    int64_t count = 1;
    for (int64_t i = 1; i < sorted.size; i++) {
        if (sorted.data[i] != sorted.data[i - 1]) count++;
    }
    int64_t shape[1] = {count};
    LpTensor r = _lp_tensor_alloc(shape, 1);
    r.data[0] = sorted.data[0];
    int64_t j = 1;
    for (int64_t i = 1; i < sorted.size; i++) {
        if (sorted.data[i] != sorted.data[i - 1])
            r.data[j++] = sorted.data[i];
    }
    if (sorted.owns_data) free(sorted.data);
    return r;
}

/* tensor.take(t, indices_tensor) */
static inline LpTensor lp_tensor_take(LpTensor a, LpTensor indices) {
    int64_t shape[1] = {indices.size};
    LpTensor r = _lp_tensor_alloc(shape, 1);
    if (r.data && a.data && indices.data) {
        for (int64_t i = 0; i < indices.size; i++) {
            int64_t idx = (int64_t)indices.data[i];
            r.data[i] = (idx >= 0 && idx < a.size) ? a.data[idx] : 0.0;
        }
    }
    return r;
}

/* tensor.searchsorted(a, val) */
static inline int64_t lp_tensor_searchsorted(LpTensor a, double val) {
    int64_t lo = 0, hi = a.size;
    while (lo < hi) {
        int64_t mid = lo + (hi - lo) / 2;
        if (a.data[mid] < val) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

/* tensor.clip(t, lo, hi) — alias for clamp */
static inline LpTensor lp_tensor_clip(LpTensor a, double lo, double hi) {
    return lp_tensor_clamp(a, lo, hi);
}

/* tensor.power(t, p) — alias for pow_t */
static inline LpTensor lp_tensor_power(LpTensor a, double p) {
    return lp_tensor_pow_t(a, p);
}

/* tensor.where(cond, x, y) */
static inline LpTensor lp_tensor_where(LpTensor cond, LpTensor x, LpTensor y) {
    int64_t n = cond.size;
    LpTensor r = _lp_tensor_alloc(cond.shape, cond.ndim);
    if (r.data && cond.data && x.data && y.data) {
        for (int64_t i = 0; i < n; i++)
            r.data[i] = (cond.data[i] != 0.0) ? x.data[i] : y.data[i];
    }
    return r;
}

/* ================================================================
 * COMPARISON OPERATIONS
 * ================================================================ */

#define LP_TENSOR_CMPOP(name, op)                                              \
static inline LpTensor lp_tensor_##name(LpTensor a, LpTensor b) {             \
    int64_t out_shape[LP_TENSOR_MAX_NDIM]; int64_t out_ndim;                   \
    if (_lp_tensor_broadcast_shape(a.shape, a.ndim, b.shape, b.ndim,           \
                                    out_shape, &out_ndim) != 0)                \
        return lp_tensor_zeros_1d(0);                                          \
    LpTensor r = _lp_tensor_alloc(out_shape, out_ndim);                        \
    if (r.data && a.data && b.data) {                                          \
        for (int64_t i = 0; i < r.size; i++) {                                 \
            int64_t ai = _lp_tensor_broadcast_index(i, out_shape, out_ndim,    \
                a.shape, a.strides, a.ndim);                                   \
            int64_t bi = _lp_tensor_broadcast_index(i, out_shape, out_ndim,    \
                b.shape, b.strides, b.ndim);                                   \
            r.data[i] = (a.data[ai] op b.data[bi]) ? 1.0 : 0.0;              \
        }                                                                      \
    }                                                                          \
    return r;                                                                  \
}

LP_TENSOR_CMPOP(eq, ==)
LP_TENSOR_CMPOP(ne, !=)
LP_TENSOR_CMPOP(gt, >)
LP_TENSOR_CMPOP(lt, <)
LP_TENSOR_CMPOP(ge, >=)
LP_TENSOR_CMPOP(le, <=)

#undef LP_TENSOR_CMPOP

/* Count operations (backward compat with numpy) */
static inline int64_t lp_tensor_count_greater(LpTensor a, double val) {
    int64_t c = 0;
    for (int64_t i = 0; i < a.size; i++) if (a.data[i] > val) c++;
    return c;
}
static inline int64_t lp_tensor_count_less(LpTensor a, double val) {
    int64_t c = 0;
    for (int64_t i = 0; i < a.size; i++) if (a.data[i] < val) c++;
    return c;
}
static inline int64_t lp_tensor_count_equal(LpTensor a, double val) {
    int64_t c = 0;
    for (int64_t i = 0; i < a.size; i++) if (a.data[i] == val) c++;
    return c;
}

/* ================================================================
 * UTILITY FUNCTIONS
 * ================================================================ */

/* tensor.clone(t) — deep copy */
static inline LpTensor lp_tensor_clone(LpTensor a) {
    LpTensor r = _lp_tensor_alloc(a.shape, a.ndim);
    if (r.data && a.data) {
        if (_lp_tensor_is_contiguous(&a)) {
            memcpy(r.data, a.data, a.size * sizeof(double));
        } else {
            for (int64_t i = 0; i < a.size; i++) {
                int64_t idx[LP_TENSOR_MAX_NDIM];
                _lp_tensor_unravel(i, a.shape, a.ndim, idx);
                r.data[i] = a.data[_lp_tensor_ravel(idx, a.strides, a.ndim)];
            }
        }
    }
    return r;
}

/* tensor.ndim(t) */
static inline int64_t lp_tensor_ndim_of(LpTensor a) { return a.ndim; }

/* tensor.numel(t) */
static inline int64_t lp_tensor_numel(LpTensor a) { return a.size; }

/* tensor.len(t) — backward compat: first dim size */
static inline int64_t lp_tensor_len(LpTensor a) { return a.size; }

/* tensor.is_contiguous(t) */
static inline int lp_tensor_is_contiguous(LpTensor a) {
    return _lp_tensor_is_contiguous(&a);
}

/* tensor.free(t) */
static inline void lp_tensor_free(LpTensor *t) {
    if (t->owns_data && t->data) {
        free(t->data);
    }
    t->data = NULL;
    t->size = 0;
    t->ndim = 0;
}

/* tensor.print(t) — pretty print */
static inline void _lp_tensor_print_elem(double v) {
    if (v == (int64_t)v && fabs(v) < 1e15)
        printf("%" PRId64, (int64_t)v);
    else
        printf("%g", v);
}

static inline void _lp_tensor_print_recursive(const LpTensor *t, int64_t dim,
                                                int64_t offset, int indent) {
    if (dim == t->ndim - 1) {
        /* Last dimension: print elements */
        printf("[");
        for (int64_t i = 0; i < t->shape[dim]; i++) {
            if (i > 0) printf(", ");
            _lp_tensor_print_elem(t->data[offset + i * t->strides[dim]]);
        }
        printf("]");
    } else {
        printf("[");
        for (int64_t i = 0; i < t->shape[dim]; i++) {
            if (i > 0) {
                printf(",\n");
                for (int j = 0; j <= indent; j++) printf(" ");
            }
            _lp_tensor_print_recursive(t, dim + 1,
                offset + i * t->strides[dim], indent + 1);
        }
        printf("]");
    }
}

static inline void lp_tensor_print(LpTensor t) {
    if (t.ndim == 0 || t.size == 0) {
        printf("[]\n");
        return;
    }
    if (t.ndim == 1) {
        /* Simple 1D print */
        printf("[");
        for (int64_t i = 0; i < t.size; i++) {
            if (i > 0) printf(", ");
            _lp_tensor_print_elem(t.data[i * t.strides[0]]);
        }
        printf("]\n");
    } else {
        _lp_tensor_print_recursive(&t, 0, 0, 0);
        printf("\n");
    }
}

/* tensor.shape(t) — print shape as list */
static inline void lp_tensor_shape_print(LpTensor t) {
    printf("(");
    for (int64_t i = 0; i < t.ndim; i++) {
        if (i > 0) printf(", ");
        printf("%" PRId64, t.shape[i]);
    }
    printf(")\n");
}

/* tensor.median — all (implementation after sort is defined) */
static inline double lp_tensor_median_all(LpTensor a) {
    if (!a.data || a.size == 0) return 0.0;
    LpTensor sorted = lp_tensor_sort(a);
    double med;
    if (a.size % 2 == 0)
        med = (sorted.data[a.size / 2 - 1] + sorted.data[a.size / 2]) / 2.0;
    else
        med = sorted.data[a.size / 2];
    if (sorted.owns_data) free(sorted.data);
    return med;
}

/* ================================================================
 * BACKWARD COMPATIBILITY — lp_np_* aliases
 * These macros allow old codegen (numpy module) to work unchanged.
 * ================================================================ */

/* Creation */
#define lp_np_zeros(n)               lp_tensor_zeros_1d(n)
#define lp_np_ones(n)                lp_tensor_ones_1d(n)
#define lp_np_arange(a,b,c)         lp_tensor_arange(a,b,c)
#define lp_np_linspace(a,b,n)       lp_tensor_linspace(a,b,n)
#define lp_np_from_doubles           lp_tensor_from_doubles
#define lp_np_eye(n)                lp_tensor_eye(n)

/* Reductions (return scalar double) */
#define lp_np_sum(a)                lp_tensor_sum_all(a)
#define lp_np_mean(a)               lp_tensor_mean_all(a)
#define lp_np_min(a)                lp_tensor_min_all(a)
#define lp_np_max(a)                lp_tensor_max_all(a)
#define lp_np_std(a)                lp_tensor_std_all(a)
#define lp_np_var(a)                lp_tensor_var_all(a)
#define lp_np_median(a)             lp_tensor_median_all(a)
#define lp_np_dot(a,b)              lp_tensor_dot(a,b)
#define lp_np_norm(a)               lp_tensor_norm_all(a)
#define lp_np_argmax(a)             lp_tensor_argmax_all(a)
#define lp_np_argmin(a)             lp_tensor_argmin_all(a)
#define lp_np_len(a)                lp_tensor_len(a)

/* Element-wise (return tensor) */
#define lp_np_add(a,b)              lp_tensor_add(a,b)
#define lp_np_sub(a,b)              lp_tensor_sub(a,b)
#define lp_np_mul(a,b)              lp_tensor_mul(a,b)
#define lp_np_div(a,b)              lp_tensor_div(a,b)
#define lp_np_scale(a,s)            lp_tensor_scale(a,s)
#define lp_np_add_scalar(a,s)       lp_tensor_add_scalar(a,s)
#define lp_np_sqrt_arr(a)           lp_tensor_sqrt_t(a)
#define lp_np_abs_arr(a)            lp_tensor_abs_t(a)
#define lp_np_sin_arr(a)            lp_tensor_sin_t(a)
#define lp_np_cos_arr(a)            lp_tensor_cos_t(a)
#define lp_np_exp_arr(a)            lp_tensor_exp_t(a)
#define lp_np_log_arr(a)            lp_tensor_log_t(a)

/* Shape */
#define lp_np_reshape2d(a,r,c)      lp_tensor_reshape2d(a,r,c)
#define lp_np_transpose(a)          lp_tensor_transpose_2d(a)
#define lp_np_flatten(a)            lp_tensor_flatten(a)
#define lp_np_cumsum(a)             lp_tensor_cumsum(a)

/* Array ops */
#define lp_np_sort(a)               lp_tensor_sort(a)
#define lp_np_reverse(a)            lp_tensor_reverse(a)
#define lp_np_clip(a,lo,hi)         lp_tensor_clip(a,lo,hi)
#define lp_np_power(a,p)            lp_tensor_power(a,p)
#define lp_np_matmul(a,b)           lp_tensor_matmul(a,b)
#define lp_np_diag(a)               lp_tensor_diag(a)
#define lp_np_take(a,b)             lp_tensor_take(a,b)
#define lp_np_concatenate(a,b)      lp_tensor_concatenate(a,b)
#define lp_np_unique(a)             lp_tensor_unique(a)
#define lp_np_diff(a)               lp_tensor_diff(a)
#define lp_np_searchsorted(a,v)     lp_tensor_searchsorted(a,v)
#define lp_np_where(c,x,y)         lp_tensor_where(c,x,y)

/* Count ops */
#define lp_np_count_greater(a,v)    lp_tensor_count_greater(a,v)
#define lp_np_count_less(a,v)       lp_tensor_count_less(a,v)
#define lp_np_count_equal(a,v)      lp_tensor_count_equal(a,v)

/* Print */
#define lp_np_print(a)              lp_tensor_print(a)

/* Free */
#define lp_np_free(a)               lp_tensor_free(a)

#endif /* LP_TENSOR_H */
