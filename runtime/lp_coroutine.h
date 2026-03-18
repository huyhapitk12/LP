/*
 * lp_coroutine.h - Coroutine Support for Async/Await
 * 
 * Provides a simple coroutine system for LP's async/await feature.
 * This enables true asynchronous execution instead of just syntactic sugar.
 */

#ifndef LP_COROUTINE_H
#define LP_COROUTINE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <stdbool.h>

/* Maximum coroutines in the scheduler */
#define LP_MAX_COROUTINES 256

/* Coroutine states */
typedef enum {
    LP_CORO_CREATED,     /* Created but not started */
    LP_CORO_RUNNING,     /* Currently executing */
    LP_CORO_SUSPENDED,   /* Yielded, waiting to resume */
    LP_CORO_FINISHED,    /* Execution completed */
    LP_CORO_ERROR        /* Error during execution */
} LpCoroState;

/* Forward declaration */
typedef struct LpCoroutine LpCoroutine;

/* Coroutine function signature */
typedef void (*LpCoroFunc)(LpCoroutine* coro, void* arg);

/* Coroutine structure */
struct LpCoroutine {
    int64_t id;                   /* Unique coroutine ID */
    LpCoroState state;            /* Current state */
    LpCoroFunc func;              /* Coroutine function */
    void* arg;                    /* User argument */
    void* result;                 /* Result value */
    char* error;                  /* Error message if any */
    ucontext_t context;           /* Execution context */
    ucontext_t caller_context;    /* Context to return to */
    void* stack;                  /* Coroutine stack */
    size_t stack_size;            /* Stack size */
    int64_t yield_value_i;        /* Integer yield value */
    double yield_value_f;         /* Float yield value */
    void* yield_value_p;          /* Pointer yield value */
    int yield_type;               /* 0=none, 1=int, 2=float, 3=pointer */
};

/* Coroutine scheduler */
typedef struct {
    LpCoroutine* coroutines[LP_MAX_COROUTINES];
    int64_t count;
    int64_t current_id;
    ucontext_t main_context;
} LpCoroSched;

/* Global scheduler (singleton) */
static LpCoroSched* _lp_coro_sched = NULL;

/* Initialize the coroutine scheduler */
static inline void lp_coro_init(void) {
    if (_lp_coro_sched) return;
    _lp_coro_sched = (LpCoroSched*)calloc(1, sizeof(LpCoroSched));
    _lp_coro_sched->current_id = 0;
}

/* Get the global scheduler */
static inline LpCoroSched* lp_coro_get_sched(void) {
    if (!_lp_coro_sched) lp_coro_init();
    return _lp_coro_sched;
}

/* Create a new coroutine */
static inline LpCoroutine* lp_coro_create(LpCoroFunc func, void* arg) {
    LpCoroSched* sched = lp_coro_get_sched();
    if (sched->count >= LP_MAX_COROUTINES) return NULL;
    
    LpCoroutine* coro = (LpCoroutine*)calloc(1, sizeof(LpCoroutine));
    if (!coro) return NULL;
    
    coro->id = ++sched->current_id;
    coro->state = LP_CORO_CREATED;
    coro->func = func;
    coro->arg = arg;
    coro->stack_size = 64 * 1024; /* 64KB stack */
    coro->stack = malloc(coro->stack_size);
    
    if (!coro->stack) {
        free(coro);
        return NULL;
    }
    
    /* Find empty slot */
    for (int i = 0; i < LP_MAX_COROUTINES; i++) {
        if (!sched->coroutines[i]) {
            sched->coroutines[i] = coro;
            sched->count++;
            break;
        }
    }
    
    return coro;
}

/* Wrapper function for context switch */
static inline void _lp_coro_entry(LpCoroutine* coro) {
    if (coro->func) {
        coro->func(coro, coro->arg);
    }
    coro->state = LP_CORO_FINISHED;
    setcontext(&coro->caller_context);
}

/* Start or resume a coroutine */
static inline void* lp_coro_resume(LpCoroutine* coro) {
    if (!coro) return NULL;
    
    if (coro->state == LP_CORO_FINISHED || coro->state == LP_CORO_ERROR) {
        return coro->result;
    }
    
    coro->state = LP_CORO_RUNNING;
    
    /* Initialize context if first run */
    if (coro->state == LP_CORO_CREATED) {
        getcontext(&coro->context);
        coro->context.uc_stack.ss_sp = coro->stack;
        coro->context.uc_stack.ss_size = coro->stack_size;
        coro->context.uc_link = &coro->caller_context;
        makecontext(&coro->context, (void(*)(void))_lp_coro_entry, 1, coro);
    }
    
    /* Save current context and switch to coroutine */
    swapcontext(&coro->caller_context, &coro->context);
    
    return coro->result;
}

/* Yield from a coroutine */
static inline void lp_coro_yield(LpCoroutine* coro) {
    if (!coro) return;
    coro->state = LP_CORO_SUSPENDED;
    swapcontext(&coro->context, &coro->caller_context);
}

/* Yield with value */
static inline void lp_coro_yield_int(LpCoroutine* coro, int64_t val) {
    if (!coro) return;
    coro->yield_value_i = val;
    coro->yield_type = 1;
    lp_coro_yield(coro);
}

static inline void lp_coro_yield_float(LpCoroutine* coro, double val) {
    if (!coro) return;
    coro->yield_value_f = val;
    coro->yield_type = 2;
    lp_coro_yield(coro);
}

/* Get yield value */
static inline int64_t lp_coro_get_yield_int(LpCoroutine* coro) {
    return coro ? coro->yield_value_i : 0;
}

static inline double lp_coro_get_yield_float(LpCoroutine* coro) {
    return coro ? coro->yield_value_f : 0.0;
}

/* Set result */
static inline void lp_coro_set_result(LpCoroutine* coro, void* result) {
    if (coro) coro->result = result;
}

/* Check if coroutine is done */
static inline int lp_coro_is_done(LpCoroutine* coro) {
    return coro ? (coro->state == LP_CORO_FINISHED || coro->state == LP_CORO_ERROR) : 1;
}

/* Get coroutine state */
static inline LpCoroState lp_coro_state(LpCoroutine* coro) {
    return coro ? coro->state : LP_CORO_ERROR;
}

/* Free a coroutine */
static inline void lp_coro_free(LpCoroutine* coro) {
    if (!coro) return;
    
    /* Remove from scheduler */
    LpCoroSched* sched = lp_coro_get_sched();
    for (int i = 0; i < LP_MAX_COROUTINES; i++) {
        if (sched->coroutines[i] == coro) {
            sched->coroutines[i] = NULL;
            sched->count--;
            break;
        }
    }
    
    if (coro->stack) free(coro->stack);
    if (coro->error) free(coro->error);
    free(coro);
}

/* Run all coroutines until complete */
static inline void lp_coro_run_all(void) {
    LpCoroSched* sched = lp_coro_get_sched();
    int has_pending = 1;
    
    while (has_pending) {
        has_pending = 0;
        for (int i = 0; i < LP_MAX_COROUTINES; i++) {
            LpCoroutine* coro = sched->coroutines[i];
            if (coro && !lp_coro_is_done(coro)) {
                has_pending = 1;
                lp_coro_resume(coro);
            }
        }
    }
}

/* Await a coroutine - blocks until complete */
static inline void* lp_coro_await(LpCoroutine* coro) {
    if (!coro) return NULL;
    
    while (!lp_coro_is_done(coro)) {
        lp_coro_resume(coro);
    }
    
    return coro->result;
}

/* Cleanup all coroutines */
static inline void lp_coro_cleanup(void) {
    LpCoroSched* sched = lp_coro_get_sched();
    for (int i = 0; i < LP_MAX_COROUTINES; i++) {
        if (sched->coroutines[i]) {
            lp_coro_free(sched->coroutines[i]);
        }
    }
    sched->count = 0;
}

#endif /* LP_COROUTINE_H */
