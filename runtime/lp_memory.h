#ifndef LP_MEMORY_H
#define LP_MEMORY_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lp_runtime.h"

/* 
 * Arena Allocator 
 * Useful for building up objects dynamically.
 */
typedef struct LpArena {
    char* region;
    int64_t size;
    int64_t offset;
} LpArena;

static inline LpArena* lp_memory_arena_new(int64_t size) {
    LpArena* arena = (LpArena*)malloc(sizeof(LpArena));
    arena->region = (char*)malloc(size);
    arena->size = size;
    arena->offset = 0;
    return arena;
}

static inline void* lp_memory_arena_alloc(LpArena* arena, int64_t bytes) {
    if (arena->offset + bytes > arena->size) return NULL;
    void* ptr = arena->region + arena->offset;
    arena->offset += bytes;
    return ptr;
}

static inline void lp_memory_arena_reset(LpArena* arena) {
    arena->offset = 0;
}

static inline void lp_memory_arena_free(LpArena* arena) {
    free(arena->region);
    free(arena);
}

/* 
 * Pool Allocator
 * Useful for fixed-size object allocation with high throughput.
 */
typedef struct LpPoolNode {
    struct LpPoolNode* next;
} LpPoolNode;

typedef struct LpPool {
    char* region;
    LpPoolNode* free_list;
    int64_t chunk_size;
    int64_t total_chunks;
} LpPool;

static inline LpPool* lp_memory_pool_new(int64_t chunk_size, int64_t num_chunks) {
    LpPool* pool = (LpPool*)malloc(sizeof(LpPool));
    pool->region = (char*)malloc(chunk_size * num_chunks);
    pool->chunk_size = chunk_size;
    pool->total_chunks = num_chunks;
    
    /* Initialize free list */
    pool->free_list = (LpPoolNode*)pool->region;
    LpPoolNode* curr = pool->free_list;
    for (int64_t i = 1; i < num_chunks; i++) {
        curr->next = (LpPoolNode*)(pool->region + (i * chunk_size));
        curr = curr->next;
    }
    curr->next = NULL;
    return pool;
}

static inline void* lp_memory_pool_alloc(LpPool* pool) {
    if (!pool->free_list) return NULL;
    void* ptr = pool->free_list;
    pool->free_list = pool->free_list->next;
    return ptr;
}

static inline void lp_memory_pool_free(LpPool* pool, void* ptr) {
    LpPoolNode* node = (LpPoolNode*)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
}

static inline void lp_memory_pool_destroy(LpPool* pool) {
    free(pool->region);
    free(pool);
}

#endif
