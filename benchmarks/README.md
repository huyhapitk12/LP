# LP vs C++ Benchmark Suite — 19 Benchmarks

Comprehensive performance comparison across 7 categories, measured on 5 dimensions.

## Results Summary (Linux x86-64, g++ -O2, 5× median)

### Runtime Performance

| Benchmark | Category | LP | C++ | Ratio | Winner |
|-----------|----------|----|-----|-------|--------|
| Arithmetic Loop 100M | Arithmetic | 0.0036s | 0.0039s | 0.93× | **LP** |
| Fibonacci n=38 (recursive) | Recursion | 0.0993s | 0.0817s | 1.21× | C++ |
| Quicksort n=100K | Sorting | 0.0112s | 0.0136s | 0.82× | **LP** |
| Bubble Sort n=8K | Sorting | 0.2749s | 0.2792s | 0.98× | TIE |
| Binary Search 1M queries | Search | 0.0915s | 0.1106s | 0.83× | **LP** |
| Math Float 1M (sin/cos/sqrt) | Math | 0.0086s | 0.0182s | 0.48× | **LP** |
| Matrix Multiply 300×300 | Math | 0.0125s | 0.0230s | 0.55× | **LP** |
| Prime Factorization 100K | Math | 0.0325s | 0.0341s | 0.95× | TIE |
| String Operations 200K | String | 0.0594s | 0.0660s | 0.90× | **LP** |
| Hash Map 100K | Dict | 0.0371s | 0.0119s | 3.12× | C++ |
| Knapsack DP n=300 | DP | 0.0037s | 0.0059s | 0.62× | **LP** |
| LCS DP 1000×1000 | DP | 0.0124s | 0.0107s | 1.15× | C++ |
| Dijkstra n=500 | Graph | 0.0055s | 0.0077s | 0.72× | **LP** |
| BFS 1000×1000 (list[]) | Graph | 0.0340s | 0.0185s | 1.84× | C++ |
| BFS 1000×1000 (i32[]) ✨ | Graph·opt | 0.0168s | 0.0175s | 0.96× | TIE |
| Sieve 5M (list[]) | Prime | 0.1014s | 0.0295s | 3.44× | C++ |
| Sieve 5M (i8[]) ✨ | Prime·opt | 0.0256s | 0.0301s | 0.85× | **LP** |
| Tree DFS depth-20 | Recursion | 0.0321s | 0.0049s | 6.5× | C++ |
| Memory Alloc 10K×1K | Memory | 0.1847s | 0.0108s | 17× | C++ |

**Score: LP 9 wins | 3 ties | C++ 7 wins**  
**With optimised types (i8[]/i32[]): LP 11 wins | 3 ties | C++ 5 wins**

### Compile Speed

| Benchmark | LP frontend | g++ -O2 | Speedup |
|-----------|------------|---------|---------|
| Most files | ~5–6 ms | 400–800 ms | **80–145×** |
| knapsack | ~34 ms | 751 ms | **22×** |

LP frontend = parse + semantic check + codegen + write C file.

### Why LP Wins on Math

LP automatically applies `__attribute__((target("avx2,fma"), optimize("O3,unroll-loops")))` to all hot functions. This enables AVX2/FMA SIMD that `g++ -O2` does **not** activate unless `-march=native` is explicitly passed.

### Known LP Weaknesses

1. **Memory allocation churn (17×)**: Each `[0]*N` allocation calls `malloc()`. Fix: arena/pool auto-allocation for repeated alloc patterns.
2. **Tree DFS with large stacks (6.5×)**: Use `i32[]` annotation to halve memory and improve cache performance.
3. **Hash map (3.1×)**: LP generic dict vs optimised `std::unordered_map`. Fix: typed dict specialisation.

### Type Optimisation Guide

```lp
# Slow: LpVal = 16 bytes/element
sieve: list = [1] * (n + 1)   # 5M × 16B = 80MB

# Fast: int8_t = 1 byte/element  
sieve: i8[] = [1] * (n + 1)   # 5M × 1B = 5MB  → LP beats C++

# Fast: int32_t = 4 bytes/element
dist: i32[] = [0] * total      # 2× denser than int64 → better cache
```

## Running Benchmarks

```bash
# Compile LP benchmarks
for f in bench_*.lp; do lp "$f" --gcc -c "${f%.lp}"; done

# Compile C++ benchmarks  
for f in bench_*.cpp; do g++ -O2 -std=c++17 -o "${f%.cpp}_cpp" "$f" -lm; done

# Run a pair
./bench_sieve && ./bench_sieve_cpp
```

## Benchmark Descriptions

- `bench_arith` — Arithmetic loop: `sum += i` for 100M iterations
- `bench_fib` — Fibonacci(38) via naive recursion (39M calls)
- `bench_qsort` — Iterative quicksort on 100K pseudo-random integers
- `bench_sort` — Bubble sort O(n²) on 8K integers
- `bench_bsearch` — Binary search: 1M queries on sorted array of 1M
- `bench_mathfloat` — 1M iterations of sin/cos/sqrt/log
- `bench_matmul` — Dense matrix multiplication 300×300
- `bench_primes` — Prime factorisation of 100K integers
- `bench_strops` — 200K iterations: upper/lower/find on string
- `bench_dict` — 100K hash map insert + 10K lookup
- `bench_knapsack` — 0/1 Knapsack DP n=300 W=3000
- `bench_lcs` — Longest Common Subsequence DP 1000×1000
- `bench_dijkstra` — Dijkstra O(n²) shortest path n=500
- `bench_bfs` — BFS on 1000×1000 grid (1M nodes)
- `bench_bfs_i32` — BFS with i32[] arrays (optimised memory)
- `bench_sieve` — Sieve of Eratosthenes up to 5M
- `bench_sieve_i8` — Sieve with i8[] byte array (optimised)
- `bench_treedfs` — Iterative DFS of complete binary tree depth 20
- `bench_alloc` — 10K allocations of 1K-element arrays
