# Fibonacci Benchmark - LP Language
# Run: lp fibonacci.lp

def fibonacci(n: int) -> int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

# Compute Fibonacci numbers
for i in range(10):
    print(fibonacci(i))

# Benchmark: Fibonacci(35)
import time
start: float = time.time()
result: int = fibonacci(35)
elapsed: float = time.time() - start

print("fibonacci(35) =")
print(result)
print("Time (seconds):")
print(elapsed)
