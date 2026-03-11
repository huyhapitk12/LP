import memory

class Particle:
    id: int = 0
    velocity: float = 0.0
    life: int = 0

def init_particle(slot: ptr, particle_id: int, velocity: float, life: int) -> int:
    particle = memory.cast(slot, Particle)
    particle.id = particle_id
    particle.velocity = velocity
    particle.life = life
    return particle.id

def tick_particle(slot: ptr) -> float:
    particle = memory.cast(slot, Particle)
    particle.life = particle.life - 1
    particle.velocity = particle.velocity * 1.1
    return particle.velocity

def main():
    print("Allocating a frame arena...")

    arena: ptr = memory.arena_new(256)

    p1: ptr = memory.arena_alloc(arena, 16)
    p2: ptr = memory.arena_alloc(arena, 16)

    init_particle(p1, 1, 3.5, 5)
    init_particle(p2, 2, 1.75, 4)

    print(tick_particle(p1))
    print(tick_particle(p2))

    print("Resetting arena for the next frame.")
    memory.arena_reset(arena)

    p3: ptr = memory.arena_alloc(arena, 16)
    init_particle(p3, 3, 9.0, 2)
    print(tick_particle(p3))

    memory.arena_free(arena)
    print("Arena released.")

main()
