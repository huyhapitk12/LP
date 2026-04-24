#ifndef LP_ARRAY_H
#define LP_ARRAY_H

/*
 * Backward compatibility layer.
 * LpArray is now a typedef for LpTensor.
 * All lp_np_* functions are #defined as macros in lp_tensor.h.
 */
#include "lp_tensor.h"

typedef LpTensor LpArray;

#endif /* LP_ARRAY_H */
