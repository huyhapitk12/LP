#ifndef LP_EXPORTED_API_H
#define LP_EXPORTED_API_H

#include <stdint.h>

#if defined(_WIN32)
  #define LP_EXTERN __declspec(dllexport)
#else
  #define LP_EXTERN __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

LP_EXTERN int64_t lp_add(int64_t lp_a, int64_t lp_b);

#ifdef __cplusplus
}
#endif

#endif /* LP_EXPORTED_API_H */
