/*
 * LP Native Set — Zero-Overhead C Hash Set Implementation
 * Maps Python's set to a native C open-addressing hash table.
 * Built on top of lp_dict.h (keys only).
 */

#ifndef LP_SET_H
#define LP_SET_H

#include "lp_dict.h"

typedef struct {
    LpDict *dict;
} LpSet;

static inline LpSet* lp_set_new(void) {
    LpSet *s = (LpSet*)malloc(sizeof(LpSet));
    s->dict = lp_dict_new(); /* We use the dict to store keys, value is ignored (NULL) */
    return s;
}

static inline void lp_set_free(LpSet *s) {
    if (!s) return;
    lp_dict_free(s->dict);
    free(s);
}

static inline void lp_set_add(LpSet *s, const char *key) {
    /* Set value to NULL simply to mark occupation */
    lp_dict_set(s->dict, key, lp_val_null());
}

static inline int lp_set_contains(LpSet *s, const char *key) {
    return lp_dict_contains(s->dict, key);
}

static inline void lp_set_print(LpSet *s) {
    printf("{");
    int first = 1;
    for (int64_t i = 0; i < s->dict->capacity; i++) {
        if (s->dict->entries[i].is_occupied) {
            if (!first) printf(", ");
            printf("\"%s\"", s->dict->entries[i].key);
            first = 0;
        }
    }
    printf("}");
}

#endif /* LP_SET_H */
