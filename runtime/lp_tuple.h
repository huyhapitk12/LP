#ifndef LP_TUPLE_H
#define LP_TUPLE_H

#include <stdint.h>
#include <stdlib.h>

/* Tuples in Python are immutable arrays.
   In LP, we can represent them as a specialized array or a struct
   if the types are mixed. For now, a generic array approach works. */

typedef struct {
    void **items;
    int count;
} LpTuple;

static inline LpTuple* lp_tuple_new(int count) {
    LpTuple *t = (LpTuple *)malloc(sizeof(LpTuple));
    if (!t) return NULL;
    t->count = count;
    t->items = (void **)calloc(count, sizeof(void *));
    if (!t->items && count > 0) {
        free(t);
        return NULL;
    }
    return t;
}

static inline void lp_tuple_set(LpTuple *t, int index, void *val) {
    if (!t || index < 0 || index >= t->count) return;
    t->items[index] = val;
}

static inline void *lp_tuple_get(LpTuple *t, int index) {
    if (!t || index < 0 || index >= t->count) return NULL;
    return t->items[index];
}

static inline void lp_tuple_free(LpTuple *t) {
    if (!t) return;
    free(t->items);
    free(t);
}

#endif
