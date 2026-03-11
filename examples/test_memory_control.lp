import memory

class Pair:
    left: int = 0
    right: int = 0

def test_memory_control():
    arena = memory.arena_new(64)

    first_ptr = memory.arena_alloc(arena, 16)
    first = memory.cast(first_ptr, Pair)
    first.left = 4
    first.right = 5
    if first.left + first.right != 9:
        raise "arena cast failed"

    memory.arena_reset(arena)

    second_ptr = memory.arena_alloc(arena, 16)
    second = memory.cast(second_ptr, Pair)
    second.left = 10
    second.right = 3
    if second.left - second.right != 7:
        raise "arena reset reuse failed"

    memory.arena_free(arena)

    pool = memory.pool_new(16, 2)

    pool_a_ptr = memory.pool_alloc(pool)
    pool_b_ptr = memory.pool_alloc(pool)

    pool_a = memory.cast(pool_a_ptr, Pair)
    pool_b = memory.cast(pool_b_ptr, Pair)

    pool_a.left = 7
    pool_a.right = 8
    pool_b.left = 2
    pool_b.right = 6

    if pool_a.left + pool_a.right != 15:
        raise "pool cast failed"
    if pool_b.left * pool_b.right != 12:
        raise "second pool allocation failed"

    memory.pool_free(pool, pool_a_ptr)
    memory.pool_free(pool, pool_b_ptr)
    memory.pool_destroy(pool)
