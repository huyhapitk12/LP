#ifndef LP_UNITTEST_H
#define LP_UNITTEST_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int lp_unittest_passed = 0;
static int lp_unittest_failed = 0;

static inline void lp_unittest_reset() {
  lp_unittest_passed = 0;
  lp_unittest_failed = 0;
}

/* --- Internal typed assert_equal --- */
static inline void lp_unittest_assert_equal_str(const char *a, const char *b,
                                                const char *msg) {
  if (a && b && strcmp(a, b) == 0) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected '%s', got '%s'\n",
           msg, b ? b : "NULL", a ? a : "NULL");
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_equal_int(int64_t a, int64_t b,
                                                const char *msg) {
  if (a == b) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected %lld, got %lld\n",
           msg, (long long)b, (long long)a);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_equal_float(double a, double b,
                                                  const char *msg) {
  if (a == b) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected %g, got %g\n",
           msg, b, a);
    lp_unittest_failed++;
  }
}

/* C11 Generic macro: Tự động gọi hàm so sánh đúng kiểu dữ liệu */
#define lp_unittest_assert_equal(a, b, msg)                                    \
  _Generic((a),                                                                \
      int: lp_unittest_assert_equal_int,                                       \
      int64_t: lp_unittest_assert_equal_int,                                   \
      double: lp_unittest_assert_equal_float,                                  \
      char *: lp_unittest_assert_equal_str,                                    \
      const char *: lp_unittest_assert_equal_str)(a, b, msg)

#define lp_unittest_assert_not_equal(a, b, msg)                                \
  _Generic((a),                                                                \
      int: lp_unittest_assert_not_equal_int,                                   \
      int64_t: lp_unittest_assert_not_equal_int,                               \
      double: lp_unittest_assert_not_equal_float,                              \
      char *: lp_unittest_assert_not_equal_str,                                \
      const char *: lp_unittest_assert_not_equal_str)(a, b, msg)

static inline void lp_unittest_assert_not_equal_str(const char *a,
                                                    const char *b,
                                                    const char *msg) {
  if (a && b && strcmp(a, b) != 0) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected not equal\n",
           msg);
    lp_unittest_failed++;
  }
}
static inline void lp_unittest_assert_not_equal_int(int64_t a, int64_t b,
                                                    const char *msg) {
  if (a != b) {
    lp_unittest_passed++;
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
  } else {
    lp_unittest_failed++;
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s\n", msg);
  }
}
static inline void lp_unittest_assert_not_equal_float(double a, double b,
                                                      const char *msg) {
  if (a != b) {
    lp_unittest_passed++;
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
  } else {
    lp_unittest_failed++;
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s\n", msg);
  }
}

/* --- Other asserts --- */
static inline void lp_unittest_assert_true(int expr, const char *msg) {
  if (expr) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s\n", msg);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_false(int expr, const char *msg) {
  if (!expr) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s\n", msg);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_near(double a, double b, double eps,
                                           const char *msg) {
  if (fabs(a - b) < eps) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected %g near %g (eps "
           "%g)\n",
           msg, a, b, eps);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_greater(double a, double b,
                                              const char *msg) {
  if (a > b) {
    lp_unittest_passed++;
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
  } else {
    lp_unittest_failed++;
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected %g > %g\n", msg,
           a, b);
  }
}

static inline void lp_unittest_assert_less(double a, double b,
                                           const char *msg) {
  if (a < b) {
    lp_unittest_passed++;
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
  } else {
    lp_unittest_failed++;
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected %g < %g\n", msg,
           a, b);
  }
}

static inline void lp_unittest_assert_contains(const char *str, const char *sub,
                                               const char *msg) {
  if (str && sub && strstr(str, sub) != NULL) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - '%s' not found in '%s'\n",
           msg, sub ? sub : "NULL", str ? str : "NULL");
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_none(const void *p, const char *msg) {
  if (!p) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected None/NULL\n",
           msg);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_assert_not_none(const void *p, const char *msg) {
  if (p) {
    printf("  \033[32m\xE2\x9C\x93 PASS:\033[0m %s\n", msg);
    lp_unittest_passed++;
  } else {
    printf("  \033[31m\xE2\x9C\x97 FAIL:\033[0m %s - expected not None/NULL\n",
           msg);
    lp_unittest_failed++;
  }
}

static inline void lp_unittest_summary(void) {
  int total = lp_unittest_passed + lp_unittest_failed;
  double pct = total > 0 ? (100.0 * lp_unittest_passed / total) : 0.0;
  printf("\n  Results: %d passed, %d failed (%.1f%%)\n", lp_unittest_passed, lp_unittest_failed, pct);
}

#endif