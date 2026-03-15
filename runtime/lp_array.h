#ifndef LP_ARRAY_H
#define LP_ARRAY_H

#include <stdint.h>
#include <stdlib.h>

/* Dynamic array type (mapped from numpy or standard lists) */
typedef struct {
    double *data;
    int64_t len;
    int64_t cap;
    int shape[4]; /* Basic ndim tracking */
} LpArray;

#endif /* LP_ARRAY_H */
