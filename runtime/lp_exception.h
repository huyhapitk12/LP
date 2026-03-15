#ifndef LP_EXCEPTION_H
#define LP_EXCEPTION_H

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define LP_MAX_TRY_DEPTH 64

/* Exception Structure */
typedef struct {
    int type;            /* Custom type identifier */
    const char *message; /* Error message */
} LpException;

/*
 * When LP_MAIN_FILE is defined (in the main emitted C file),
 * these globals are instantiated. Otherwise, they are extern.
 */
#ifdef LP_MAIN_FILE
jmp_buf lp_exp_env[LP_MAX_TRY_DEPTH];
int lp_exp_depth = 0;
LpException lp_current_exp = {0, NULL};
#else
extern jmp_buf lp_exp_env[LP_MAX_TRY_DEPTH];
extern int lp_exp_depth;
extern LpException lp_current_exp;
#endif

static inline void lp_raise(int type, const char *msg) {
    lp_current_exp.type = type;
    lp_current_exp.message = msg;
    if (lp_exp_depth > 0) {
        longjmp(lp_exp_env[lp_exp_depth - 1], 1);
    } else {
        fprintf(stderr, "Unhandled Exception: %s\n", msg);
        exit(1);
    }
}

static inline void lp_raise_str(const char *msg) {
    lp_raise(1, msg); /* 1 is the generic Exception type */
}

/* Code generation for try/catch now handled explicitly in codegen.c without macros */

#endif /* LP_EXCEPTION_H */
