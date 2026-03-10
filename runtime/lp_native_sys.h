/*
 * LP Native Sys Module — Zero-Overhead C Implementation
 * Maps Python's sys module to native C.
 */

#ifndef LP_NATIVE_SYS_H
#define LP_NATIVE_SYS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* sys.platform */
#ifdef _WIN32
  static const char *lp_sys_platform = "win32";
#elif __APPLE__
  static const char *lp_sys_platform = "darwin";
#elif __linux__
  static const char *lp_sys_platform = "linux";
#else
  static const char *lp_sys_platform = "unknown";
#endif

/* sys.maxsize */
static const int64_t lp_sys_maxsize = INT64_MAX;

/* sys.exit(code) */
static inline void lp_sys_exit(int64_t code) {
    exit((int)code);
}

/* sys.argv — must be initialized from main() */
static int _lp_sys_argc = 0;
static char **_lp_sys_argv = NULL;

static inline void lp_sys_init_argv(int argc, char **argv) {
    _lp_sys_argc = argc;
    _lp_sys_argv = argv;
}

static inline int64_t lp_sys_argv_len(void) {
    return (int64_t)_lp_sys_argc;
}

static inline const char *lp_sys_argv_get(int64_t index) {
    if (index < 0 || index >= _lp_sys_argc) return "";
    return _lp_sys_argv[index];
}

/* sys.getrecursionlimit() — LP has no recursion limit (native code) */
static inline int64_t lp_sys_getrecursionlimit(void) {
    return 1000000; /* effectively unlimited */
}

/* sys.getsizeof(type_id) — approximate sizes */
static inline int64_t lp_sys_getsizeof_int(void) { return sizeof(int64_t); }
static inline int64_t lp_sys_getsizeof_float(void) { return sizeof(double); }
static inline int64_t lp_sys_getsizeof_bool(void) { return sizeof(int); }
static inline int64_t lp_sys_getsizeof_ptr(void) { return sizeof(void*); }

#endif /* LP_NATIVE_SYS_H */
