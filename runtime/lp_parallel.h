/**
 * lp_parallel.h - OpenMP Parallel Computing Runtime for LP Language
 * 
 * Provides comprehensive parallel computing support:
 * - Thread management and control
 * - Scheduling policies (static, dynamic, guided, auto, runtime)
 * - Reduction operations (sum, product, min, max, etc.)
 * - Synchronization primitives (barrier, critical, atomic)
 * - Parallel algorithms
 * 
 * Usage in LP:
 *   @settings(parallel=True, threads=8, schedule="dynamic")
 *   parallel for i in range(1000000):
 *       result[i] = compute(data[i])
 */

#ifndef LP_PARALLEL_H
#define LP_PARALLEL_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#define LP_HAS_OPENMP 1
#else
#define LP_HAS_OPENMP 0
/* Stub functions when OpenMP is not available */
static int omp_get_max_threads(void) { return 1; }
static int omp_get_num_threads(void) { return 1; }
static int omp_get_thread_num(void) { return 0; }
static void omp_set_num_threads(int n) { (void)n; }
static int omp_get_num_procs(void) { return 1; }
static void omp_set_dynamic(int n) { (void)n; }
static int omp_get_dynamic(void) { return 0; }
static void omp_set_nested(int n) { (void)n; }
static int omp_get_nested(void) { return 0; }
static double omp_get_wtime(void) { return 0.0; }
static double omp_get_wtick(void) { return 1.0; }
#endif

/* ==================== Thread Management ==================== */

/**
 * Get the number of available processors/cores
 */
static inline int64_t lp_parallel_cores(void) {
    return omp_get_num_procs();
}

/**
 * Get maximum number of threads that can be used
 */
static inline int64_t lp_parallel_max_threads(void) {
    return omp_get_max_threads();
}

/**
 * Get current number of threads in parallel region
 */
static inline int64_t lp_parallel_num_threads(void) {
    return omp_get_num_threads();
}

/**
 * Get current thread ID (0-indexed)
 */
static inline int64_t lp_parallel_thread_id(void) {
    return omp_get_thread_num();
}

/**
 * Set number of threads for parallel regions
 */
static inline void lp_parallel_set_threads(int64_t n) {
    if (n > 0) {
        omp_set_num_threads((int)n);
    }
}

/**
 * Enable/disable dynamic thread adjustment
 */
static inline void lp_parallel_set_dynamic(int64_t enable) {
    omp_set_dynamic((int)enable);
}

/**
 * Check if dynamic adjustment is enabled
 */
static inline int64_t lp_parallel_is_dynamic(void) {
    return omp_get_dynamic();
}

/**
 * Enable/disable nested parallelism
 */
static inline void lp_parallel_set_nested(int64_t enable) {
    omp_set_nested((int)enable);
}

/**
 * Check if nested parallelism is enabled
 */
static inline int64_t lp_parallel_is_nested(void) {
    return omp_get_nested();
}

/* ==================== Timing ==================== */

/**
 * Get wall clock time in seconds
 */
static inline double lp_parallel_wtime(void) {
    return omp_get_wtime();
}

/**
 * Get timer resolution in seconds
 */
static inline double lp_parallel_wtick(void) {
    return omp_get_wtick();
}

/* ==================== Scheduling Policies ==================== */

typedef enum {
    LP_SCHED_STATIC = 0,    /* Default: divide work evenly */
    LP_SCHED_DYNAMIC = 1,   /* Work in chunks, good for uneven workloads */
    LP_SCHED_GUIDED = 2,    /* Decreasing chunk sizes */
    LP_SCHED_AUTO = 3,      /* Let runtime decide */
    LP_SCHED_RUNTIME = 4    /* Use OMP_SCHEDULE env variable */
} LpScheduleType;

/**
 * Get schedule type from string
 */
static inline LpScheduleType lp_schedule_from_string(const char *s) {
    if (!s) return LP_SCHED_STATIC;
    if (strcmp(s, "dynamic") == 0) return LP_SCHED_DYNAMIC;
    if (strcmp(s, "guided") == 0) return LP_SCHED_GUIDED;
    if (strcmp(s, "auto") == 0) return LP_SCHED_AUTO;
    if (strcmp(s, "runtime") == 0) return LP_SCHED_RUNTIME;
    return LP_SCHED_STATIC;
}

/**
 * Get string representation of schedule type
 */
static inline const char *lp_schedule_to_string(LpScheduleType sched) {
    switch (sched) {
        case LP_SCHED_STATIC: return "static";
        case LP_SCHED_DYNAMIC: return "dynamic";
        case LP_SCHED_GUIDED: return "guided";
        case LP_SCHED_AUTO: return "auto";
        case LP_SCHED_RUNTIME: return "runtime";
        default: return "static";
    }
}

/* ==================== Parallel Settings ==================== */

typedef struct {
    int enabled;              /* Enable parallel execution */
    int num_threads;          /* Number of threads (0 = auto) */
    LpScheduleType schedule;  /* Scheduling policy */
    int64_t chunk_size;       /* Chunk size for scheduling (0 = auto) */
    int dynamic_adjust;       /* Enable dynamic thread adjustment */
    int nested;               /* Enable nested parallelism */
} LpParallelSettings;

/* Global parallel settings */
static LpParallelSettings lp_global_parallel_settings = {
    .enabled = 1,
    .num_threads = 0,
    .schedule = LP_SCHED_STATIC,
    .chunk_size = 0,
    .dynamic_adjust = 0,
    .nested = 0
};

/**
 * Configure global parallel settings
 */
static inline void lp_parallel_configure(int64_t threads, const char *schedule, 
                                         int64_t chunk_size, int64_t dynamic, int64_t nested) {
    lp_global_parallel_settings.num_threads = (int)threads;
    lp_global_parallel_settings.schedule = lp_schedule_from_string(schedule);
    lp_global_parallel_settings.chunk_size = chunk_size;
    lp_global_parallel_settings.dynamic_adjust = (int)dynamic;
    lp_global_parallel_settings.nested = (int)nested;
    
    /* Apply settings */
    if (threads > 0) {
        omp_set_num_threads((int)threads);
    }
    omp_set_dynamic((int)dynamic);
    omp_set_nested((int)nested);
}

/**
 * Get current parallel settings
 */
static inline LpParallelSettings lp_parallel_get_settings(void) {
    return lp_global_parallel_settings;
}

/**
 * Enable/disable parallel execution
 */
static inline void lp_parallel_set_enabled(int64_t enable) {
    lp_global_parallel_settings.enabled = (int)enable;
}

/**
 * Check if parallel execution is enabled
 */
static inline int64_t lp_parallel_is_enabled(void) {
    return lp_global_parallel_settings.enabled && LP_HAS_OPENMP;
}

/* ==================== Parallel Reductions ==================== */

/**
 * Parallel sum reduction
 */
static inline int64_t lp_parallel_sum_int(int64_t *arr, int64_t n) {
    int64_t sum = 0;
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(+:sum)
    #endif
    for (int64_t i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/**
 * Parallel product reduction
 */
static inline int64_t lp_parallel_product_int(int64_t *arr, int64_t n) {
    int64_t product = 1;
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(*:product)
    #endif
    for (int64_t i = 0; i < n; i++) {
        product *= arr[i];
    }
    return product;
}

/**
 * Parallel max reduction
 */
static inline int64_t lp_parallel_max_int(int64_t *arr, int64_t n) {
    if (n <= 0) return 0;
    int64_t max_val = arr[0];
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(max:max_val)
    #endif
    for (int64_t i = 0; i < n; i++) {
        if (arr[i] > max_val) max_val = arr[i];
    }
    return max_val;
}

/**
 * Parallel min reduction
 */
static inline int64_t lp_parallel_min_int(int64_t *arr, int64_t n) {
    if (n <= 0) return 0;
    int64_t min_val = arr[0];
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(min:min_val)
    #endif
    for (int64_t i = 0; i < n; i++) {
        if (arr[i] < min_val) min_val = arr[i];
    }
    return min_val;
}

/**
 * Parallel sum reduction for doubles
 */
static inline double lp_parallel_sum_double(double *arr, int64_t n) {
    double sum = 0.0;
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(+:sum)
    #endif
    for (int64_t i = 0; i < n; i++) {
        sum += arr[i];
    }
    return sum;
}

/**
 * Parallel max reduction for doubles
 */
static inline double lp_parallel_max_double(double *arr, int64_t n) {
    if (n <= 0) return 0.0;
    double max_val = arr[0];
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(max:max_val)
    #endif
    for (int64_t i = 0; i < n; i++) {
        if (arr[i] > max_val) max_val = arr[i];
    }
    return max_val;
}

/**
 * Parallel min reduction for doubles
 */
static inline double lp_parallel_min_double(double *arr, int64_t n) {
    if (n <= 0) return 0.0;
    double min_val = arr[0];
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(min:min_val)
    #endif
    for (int64_t i = 0; i < n; i++) {
        if (arr[i] < min_val) min_val = arr[i];
    }
    return min_val;
}

/* ==================== Parallel Algorithms ==================== */

/**
 * Parallel array fill
 */
static inline void lp_parallel_fill_int(int64_t *arr, int64_t n, int64_t value) {
    #if LP_HAS_OPENMP
    #pragma omp parallel for
    #endif
    for (int64_t i = 0; i < n; i++) {
        arr[i] = value;
    }
}

/**
 * Parallel array fill for doubles
 */
static inline void lp_parallel_fill_double(double *arr, int64_t n, double value) {
    #if LP_HAS_OPENMP
    #pragma omp parallel for
    #endif
    for (int64_t i = 0; i < n; i++) {
        arr[i] = value;
    }
}

/**
 * Parallel array copy
 */
static inline void lp_parallel_copy_int(int64_t *dest, int64_t *src, int64_t n) {
    #if LP_HAS_OPENMP
    #pragma omp parallel for
    #endif
    for (int64_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}

/**
 * Parallel array transformation (map)
 */
typedef int64_t (*LpIntUnaryFunc)(int64_t);

static inline void lp_parallel_map_int(int64_t *dest, int64_t *src, int64_t n, LpIntUnaryFunc f) {
    #if LP_HAS_OPENMP
    #pragma omp parallel for
    #endif
    for (int64_t i = 0; i < n; i++) {
        dest[i] = f(src[i]);
    }
}

/**
 * Parallel count (count elements matching condition)
 */
typedef int (*LpIntPredicate)(int64_t);

static inline int64_t lp_parallel_count_int(int64_t *arr, int64_t n, LpIntPredicate pred) {
    int64_t count = 0;
    #if LP_HAS_OPENMP
    #pragma omp parallel for reduction(+:count)
    #endif
    for (int64_t i = 0; i < n; i++) {
        if (pred(arr[i])) count++;
    }
    return count;
}

/* ==================== Synchronization ==================== */

/**
 * Get OpenMP schedule pragma string for code generation
 */
static inline const char *lp_get_omp_schedule_pragma(LpScheduleType sched, int64_t chunk_size) {
    static char buf[128];
    const char *sched_str = lp_schedule_to_string(sched);
    
    if (chunk_size > 0) {
        snprintf(buf, sizeof(buf), "#pragma omp parallel for schedule(%s,%lld)", 
                 sched_str, (long long)chunk_size);
    } else {
        snprintf(buf, sizeof(buf), "#pragma omp parallel for schedule(%s)", sched_str);
    }
    return buf;
}

/* ==================== Parallel Region Info ==================== */

/**
 * Check if currently in a parallel region
 */
static inline int64_t lp_parallel_in_parallel(void) {
    return omp_get_num_threads() > 1;
}

/**
 * Get parallel execution info as string
 */
static inline const char *lp_parallel_info(void) {
    static char info[256];
    snprintf(info, sizeof(info), 
             "OpenMP: %s, Threads: %d/%d, Cores: %d, Dynamic: %s, Nested: %s",
             LP_HAS_OPENMP ? "enabled" : "disabled",
             omp_get_num_threads(),
             omp_get_max_threads(),
             omp_get_num_procs(),
             omp_get_dynamic() ? "yes" : "no",
             omp_get_nested() ? "yes" : "no");
    return info;
}

/* ==================== Thread-Local Storage ==================== */

#ifdef _OPENMP
#define LP_THREAD_LOCAL __thread
#else
#define LP_THREAD_LOCAL
#endif

/**
 * Simple thread-local counter
 */
static LP_THREAD_LOCAL int64_t lp_thread_local_counter = 0;

static inline int64_t lp_thread_local_get(void) {
    return lp_thread_local_counter;
}

static inline void lp_thread_local_set(int64_t val) {
    lp_thread_local_counter = val;
}

static inline int64_t lp_thread_local_inc(void) {
    return ++lp_thread_local_counter;
}

static inline int64_t lp_thread_local_add(int64_t val) {
    lp_thread_local_counter += val;
    return lp_thread_local_counter;
}

#endif /* LP_PARALLEL_H */
