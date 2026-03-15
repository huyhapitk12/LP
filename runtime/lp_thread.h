#ifndef LP_THREAD_H
#define LP_THREAD_H

#include <stdlib.h>
#include <stdint.h>
#include "lp_runtime.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef int64_t (*LpThreadProc)(void*);

typedef struct LpThreadImpl {
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t handle;
#endif
    LpThreadProc func;
    void *arg;
    int64_t result;
} LpThreadImpl;

typedef LpThreadImpl* LpThread;

#ifdef _WIN32
typedef CRITICAL_SECTION* LpLock;

static DWORD WINAPI _lp_thread_wrap(LPVOID lpParam) {
    LpThread thread = (LpThread)lpParam;
    thread->result = (thread && thread->func) ? thread->func(thread->arg) : 0;
    return 0;
}

static inline LpThread lp_thread_spawn(LpThreadProc func, void* arg) {
    DWORD thread_id = 0;
    LpThread thread = (LpThread)calloc(1, sizeof(LpThreadImpl));
    if (!thread) return NULL;

    thread->func = func;
    thread->arg = arg;
    thread->result = 0;
    thread->handle = CreateThread(NULL, 0, _lp_thread_wrap, thread, 0, &thread_id);
    if (!thread->handle) {
        free(thread);
        return NULL;
    }
    return thread;
}

static inline int64_t lp_thread_join(LpThread thread) {
    int64_t result = 0;
    if (!thread) return 0;

    WaitForSingleObject(thread->handle, INFINITE);
    result = thread->result;
    CloseHandle(thread->handle);
    free(thread);
    return result;
}

static inline LpLock lp_thread_lock_init(void) {
    LpLock lock = (LpLock)malloc(sizeof(CRITICAL_SECTION));
    if (!lock) return NULL;
    InitializeCriticalSection(lock);
    return lock;
}

static inline void lp_thread_lock_acquire(LpLock lock) {
    if (lock) EnterCriticalSection(lock);
}

static inline void lp_thread_lock_release(LpLock lock) {
    if (lock) LeaveCriticalSection(lock);
}

#else
typedef pthread_mutex_t* LpLock;

static void* _lp_thread_wrap(void* lpParam) {
    LpThread thread = (LpThread)lpParam;
    thread->result = (thread && thread->func) ? thread->func(thread->arg) : 0;
    return NULL;
}

static inline LpThread lp_thread_spawn(LpThreadProc func, void* arg) {
    LpThread thread = (LpThread)calloc(1, sizeof(LpThreadImpl));
    if (!thread) return NULL;

    thread->func = func;
    thread->arg = arg;
    thread->result = 0;
    if (pthread_create(&thread->handle, NULL, _lp_thread_wrap, thread) != 0) {
        free(thread);
        return NULL;
    }
    return thread;
}

static inline int64_t lp_thread_join(LpThread thread) {
    int64_t result = 0;
    if (!thread) return 0;

    pthread_join(thread->handle, NULL);
    result = thread->result;
    free(thread);
    return result;
}

static inline LpLock lp_thread_lock_init(void) {
    LpLock lock = (LpLock)malloc(sizeof(pthread_mutex_t));
    if (!lock) return NULL;
    pthread_mutex_init(lock, NULL);
    return lock;
}

static inline void lp_thread_lock_acquire(LpLock lock) {
    if (lock) pthread_mutex_lock(lock);
}

static inline void lp_thread_lock_release(LpLock lock) {
    if (lock) pthread_mutex_unlock(lock);
}
#endif

#endif
