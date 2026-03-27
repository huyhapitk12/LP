/*
 * LP Arena Allocator — Bump-pointer memory allocation
 * Ultra-fast allocation for short-lived objects.
 * Allocate many, free all at once.
 */
#ifndef LP_ARENA_H
#define LP_ARENA_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define LP_ARENA_BLOCK_SIZE (1024 * 1024) /* 1 MB blocks */

typedef struct LpArenaBlock {
    struct LpArenaBlock *next;
    size_t size;
    size_t used;
    char data[];
} LpArenaBlock;

typedef struct {
    LpArenaBlock *head;
    LpArenaBlock *current;
    size_t total_allocated;
} LpArena;

static inline LpArenaBlock *lp_arena_block_new(size_t size) {
    LpArenaBlock *b = (LpArenaBlock *)malloc(sizeof(LpArenaBlock) + size);
    if (!b) return NULL;
    b->next = NULL;
    b->size = size;
    b->used = 0;
    return b;
}

static inline LpArena *lp_arena_new(void) {
    LpArena *a = (LpArena *)malloc(sizeof(LpArena));
    if (!a) return NULL;
    a->head = lp_arena_block_new(LP_ARENA_BLOCK_SIZE);
    a->current = a->head;
    a->total_allocated = 0;
    return a;
}

static inline void *lp_arena_alloc(LpArena *a, size_t size) {
    if (!a || !a->current) return NULL;
    /* Align to 16 bytes */
    size = (size + 15) & ~(size_t)15;
    if (a->current->used + size > a->current->size) {
        size_t block_size = size > LP_ARENA_BLOCK_SIZE ? size : LP_ARENA_BLOCK_SIZE;
        LpArenaBlock *nb = lp_arena_block_new(block_size);
        if (!nb) return NULL;
        a->current->next = nb;
        a->current = nb;
    }
    void *ptr = a->current->data + a->current->used;
    a->current->used += size;
    a->total_allocated += size;
    return ptr;
}

static inline void *lp_arena_calloc(LpArena *a, size_t count, size_t size) {
    void *p = lp_arena_alloc(a, count * size);
    if (p) memset(p, 0, count * size);
    return p;
}

static inline char *lp_arena_strdup(LpArena *a, const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *p = (char *)lp_arena_alloc(a, len);
    if (p) memcpy(p, s, len);
    return p;
}

static inline void lp_arena_reset(LpArena *a) {
    if (!a) return;
    LpArenaBlock *b = a->head;
    while (b) { b->used = 0; b = b->next; }
    a->current = a->head;
    a->total_allocated = 0;
}

static inline void lp_arena_free(LpArena *a) {
    if (!a) return;
    LpArenaBlock *b = a->head;
    while (b) {
        LpArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    free(a);
}

#endif /* LP_ARENA_H */
