# Performance Memory Parsing Engine
import memory
import math

class Particle:
    velocity: float = 0.0
    life: int = 10

def simulate_particle(p_ptr: ptr) -> float:
    # Memory blocks are mutated explicitly 
    p = memory.cast(p_ptr, Particle)
    p.life = p.life - 1
    p.velocity = p.velocity * 1.5
    return p.velocity

def main():
    print("Instantiating memory Arena!")
    
    # Grabs 100kb OS Memory Region for instant evaluations
    arena: ptr = memory.arena_new(102400)
    
    # Populate the arena with Particle structs instantly without Garbage delays 
    p1: ptr = memory.arena_alloc(arena, 16) # Allocate size footprint manually
    
    # Memory blocks get completely cleared manually at the end!
    memory.arena_free(arena)
    
    print("Wiped Arena buffers without GC latency!")

main()
