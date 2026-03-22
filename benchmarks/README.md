# LP vs C++ Benchmark Suite

Seven algorithmic benchmarks comparing LP v0.1.0 (GCC backend) against g++ -O2.

## Results Summary (Linux x86-64, 5× median)

| Benchmark         | LP      | C++     | Ratio  | Notes |
|-------------------|---------|---------|--------|-------|
| Arithmetic Loop   | 0.0035s | 0.0031s | 1.13×  | effectively tied |
| Fibonacci n=38    | 0.0872s | 0.0800s | 1.09×  | effectively tied |
| Bubble Sort n=8K  | 0.2675s | 0.2630s | 1.02×  | effectively tied |
| Matrix Mul 300²   | 0.0115s | 0.0212s | **0.54×** | LP wins (AVX2/FMA auto) |
| Sieve 5M (list[]) | 0.0835s | 0.0284s | 2.94×  | use i8[] instead |
| Sieve 5M (i8[])   | 0.0236s | 0.0258s | **0.91×** | LP wins with i8[] |
| Knapsack DP       | 0.0040s | 0.0056s | **0.71×** | LP wins |
| BFS 1000×1000     | 0.0299s | 0.0166s | 1.80×  | queue overhead |

## Running

```bash
# Compile LP benchmarks
for f in bench_*.lp; do lp "$f" --gcc -c "${f%.lp}"; done

# Compile C++ benchmarks
for f in bench_*.cpp; do g++ -O2 -std=c++17 -o "${f%.cpp}_cpp" "$f"; done

# Run
./bench_sieve && ./bench_sieve_cpp
```

## Key Finding

LP compiles **80–150× faster** than g++ -O2 (4–27ms vs 385–701ms).
