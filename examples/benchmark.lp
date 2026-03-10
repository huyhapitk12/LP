# Performance Benchmark - LP Language
# Run: lp benchmark.lp
# Compare: python benchmark.py

import time

# Benchmark 1: Sum of squares
def sum_of_squares(n: int) -> int:
    total: int = 0
    for i in range(n):
        total = total + i * i
    return total

start: float = time.time()
result: int = sum_of_squares(10000000)
elapsed: float = time.time() - start
print("Sum of squares (10M):")
print(result)
print("Time:")
print(elapsed)

# Benchmark 2: Loop speed
start = time.time()
counter: int = 0
for i in range(100000000):
    counter = counter + 1
elapsed = time.time() - start
print("Loop 100M iterations:")
print("Time:")
print(elapsed)

# Benchmark 3: Function call overhead
def add_one(x: int) -> int:
    return x + 1

start = time.time()
val: int = 0
for i in range(10000000):
    val = add_one(val)
elapsed = time.time() - start
print("10M function calls:")
print(val)
print("Time:")
print(elapsed)
