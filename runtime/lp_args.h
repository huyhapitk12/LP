#ifndef LP_ARGS_H
#define LP_ARGS_H

#include "lp_dict.h"
#include "lp_tuple.h"
#include <stdarg.h>

/* C Struct for managing a packed sequence of arguments (e.g. *args) */
typedef struct {
    int *types; /* generic integer type tracking since LpType is compiler-only */
    void **values;
    int count;
    int capacity;
} LpVarArgs;

static inline LpVarArgs lp_args_new(int capacity) {
    LpVarArgs args;
    args.count = 0;
    args.capacity = capacity > 0 ? capacity : 4;
    args.types = (int*)malloc(args.capacity * sizeof(int));
    args.values = (void**)malloc(args.capacity * sizeof(void*));
    return args;
}

static inline void lp_args_push(LpVarArgs *args, int type, void *value) {
    if (args->count >= args->capacity) {
        args->capacity *= 2;
        args->types = (int*)realloc(args->types, args->capacity * sizeof(int));
        args->values = (void**)realloc(args->values, args->capacity * sizeof(void*));
    }
    args->types[args->count] = type;
    args->values[args->count] = value;
    args->count++;
}

static inline void lp_args_free(LpVarArgs *args) {
    free(args->types);
    /* Note: if values are malloced dynamic types, they might need deep freeing based on 'types' */
    free(args->values);
}

#endif /* LP_ARGS_H */
