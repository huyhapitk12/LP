# Performance Guide - LP vs C++

This guide explains where LP overhead comes from and provides optimization strategies for competitive programming. The benchmark numbers below are an example snapshot, not a permanent guarantee of current performance.

## Performance Comparison

When comparing equivalent DP algorithms at the time this page was originally written:

| Language | Time | Relative |
|----------|------|----------|
| C++ | ~0.15s | 1x (baseline) |
| LP | ~0.32s | ~2x slower |
| Python | ~2-5s | 15-30x slower |

Treat these numbers as historical guidance only. Re-run the benchmark on your current compiler build before using them as a release claim.

## Why LP is Slower Than C++

### 1. Tagged Union Overhead (LpVal)

Every value in LP is wrapped in `LpVal`, a tagged union:

```c
typedef struct {
    LpValType type;  // 4 bytes: type tag
    union {
        int64_t i;   // 8 bytes: int value
        double f;    // 8 bytes: float value
        char* s;     // 8 bytes: string pointer
        // ...
    } as;
} LpVal;  // Total: ~16 bytes per value
```

**Impact:**
- **Memory**: 16 bytes per value vs 8 bytes for raw `int64_t`
- **Type checking**: Every operation checks the type tag before performing the operation
- **Indirection**: Accessing a value requires `val.as.i` instead of direct `val`

Example - Addition in LP:
```c
// LP generated code for: a + b
LpVal lp_val_add(LpVal a, LpVal b) {
    if (a.type == LP_VAL_INT && b.type == LP_VAL_INT)
        return lp_val_int(a.as.i + b.as.i);
    if ((a.type == LP_VAL_FLOAT || a.type == LP_VAL_INT) &&
        (b.type == LP_VAL_FLOAT || b.type == LP_VAL_INT)) {
        double f_a = (a.type == LP_VAL_INT) ? (double)a.as.i : a.as.f;
        double f_b = (b.type == LP_VAL_INT) ? (double)b.as.i : b.as.f;
        return lp_val_float(f_a + f_b);
    }
    // ... more type checks
}
```

Equivalent C++:
```cpp
// C++: a + b
int64_t result = a + b;  // Single CPU instruction
```

### 2. Dynamic Array Overhead (LpList)

LP lists use dynamic arrays with runtime growth:

```c
struct LpList {
    LpVal *items;   // Pointer to items
    int64_t len;    // Current length
    int64_t cap;    // Current capacity
};
```

**Impact:**
- **Bounds checking**: Every access may include bounds checking
- **Reallocation**: Growing the array may trigger `realloc()`
- **Pointer chasing**: Two levels of indirection: `list -> items -> value`

### 3. 1D Array Simulation of 2D Arrays

LP code for 2D arrays:
```lp
row_size: int = m + 2
mat: list = [0] * ((n + 2) * row_size)
val = mat[i * row_size + j]  # Index calculation
```

vs C++ native 2D:
```cpp
vector<vector<int64_t>> mat(n+2, vector<int64_t>(m+2));
val = mat[i][j];  // Direct indexing
```

The multiplication `i * row_size + j` adds overhead, though modern CPUs optimize this well.

### 4. Function Call Overhead

LP operations are function calls:
- `lp_val_add()` for `+`
- `lp_val_mul()` for `*`
- `lp_val_eq()` for `==`

Each function call has overhead (stack frame, parameter passing, return).

### 5. No SIMD Vectorization

C++ compilers can auto-vectorize loops using SIMD instructions. LP's indirect operations prevent this optimization.

## Performance Breakdown

| Factor | Overhead | Impact Level |
|--------|----------|--------------|
| Tagged union (LpVal) | 2x memory, type checks | High |
| Dynamic array (LpList) | Bounds check, indirection | Medium |
| 1D index calculation | 1 multiply-add per access | Low |
| Function call overhead | Stack operations | Medium |
| No SIMD | 2-4x slower in vectorizable code | Medium |

## Optimization Strategies

### Strategy 1: Use Native C Types When Possible

When you know the type at compile time, use explicit typing:

```lp
# Good - type is explicit
n: int = 1000000
sum: int = 0
for i in range(n):
    sum = sum + i

# Avoid - type is dynamic
data = [1, 2, 3]  # LpList with LpVal elements
```

### Strategy 2: Minimize List Operations in Hot Loops

```lp
# SLOW - Multiple list accesses per iteration
for i in range(n):
    for j in range(m):
        val = mat[i * row_size + j]
        mat[i * row_size + j] = val + 1

# FASTER - Cache value locally
for i in range(n):
    row_start: int = i * row_size
    for j in range(m):
        idx: int = row_start + j
        val: int = mat[idx]
        mat[idx] = val + 1
```

### Strategy 3: Pre-allocate Arrays

```lp
# SLOW - Appending grows array dynamically
arr: list = []
for i in range(n):
    arr.append(i)  # May trigger realloc

# FASTER - Pre-allocate with known size
arr: list = [0] * n
for i in range(n):
    arr[i] = i
```

### Strategy 4: Use DSA Module Data Structures

The DSA module provides optimized structures for competitive programming:

```lp
import dsa

# Instead of list for stack operations
stack = dsa.stack_new(10000)
dsa.stack_push(stack, value)
val = dsa.stack_pop(stack)

# Instead of list for queue operations
queue = dsa.queue_new(10000)
dsa.queue_push(queue, value)
val = dsa.queue_pop(queue)
```

### Strategy 5: Batch I/O Operations

```lp
import dsa

# Read all input at once
n: int = dsa.read_int()
arr: list = [0] * n
for i in range(n):
    arr[i] = dsa.read_int()

# Process all data
# ...

# Write all output at once
dsa.write_int_ln(result)
dsa.flush()
```

## Performance Roadmap — Approaching C++

LP's current ~2× overhead vs C++ comes from a single root cause: **every value is a 16-byte `LpVal` tagged union**, even when the type is statically known. The roadmap below addresses this in phases.

### Overhead breakdown

| Source | % of overhead | Fixed by |
|--------|--------------|----------|
| LpVal tagged union (16B/val) | ~40% | Phase 1 |
| Type check on every operation | ~25% | Phase 1 |
| LpList bounds check + pointer indirection | ~15% | Phase 2 |
| Function call overhead (`lp_val_add` etc.) | ~12% | Phase 2 |
| No SIMD / auto-vectorization | ~8% | Phase 3 |

---

### Phase 1 — Compile-time type specialization *(planned)*
**Expected result: 2.0× → 1.2–1.5× C++**

When a variable is explicitly annotated (`x: int`), the compiler will emit `int64_t` directly instead of wrapping in `LpVal`. This single change eliminates ~65% of overhead.

```lp
# Current — emits lp_val_add(a, b) — 8+ instructions
n: int = 1000
sum: int = 0
for i in range(n):
    sum = sum + i   # → lp_val_add(lp_sum, lp_i)

# After Phase 1 — emits direct C addition — 1 instruction
sum = sum + i       # → lp_sum += lp_i
```

**Typed array syntax (Phase 1 feature):**
```lp
# Planned syntax
arr: int[] = [0] * n        # → int64_t* arr = calloc(n, 8)
mat: float[] = [0.0] * n*n  # → double* mat = calloc(n*n, 8)

# 2D array via DSA module (planned)
D = dsa.int_array2d(N, N)   # raw int64_t[N][N], no LpVal overhead
```

---

### Phase 2 — Bounds check elimination + loop optimizations *(planned)*
**Expected result: 1.5× → 1.2–1.3× C++**

- **BCE in range loops:** `for i in range(n)` with `arr[i]` (same size) → compiler proves always in-bounds → removes runtime check
- **Constant propagation:** `N: int = 100` (never reassigned) → propagate `100` into loop bounds → enables GCC vectorization
- **`-O3 -march=native` for release builds:** `lp build --release` will add this flag to use AVX2/SSE4.2 of the host CPU
- **`dsa.nn_list_new(X, Y, N, K)`:** Precompute K nearest neighbors for each node — reduces 2-opt from O(N²) to O(N×K) per pass (critical for TSP)

---

### Phase 3 — SIMD + auto-vectorization *(planned)*
**Expected result: 1.3× → 1.05–1.1× C++ for numeric loops**

- Emit `__restrict__` for typed array function arguments → GCC/Clang enables auto-vectorization
- Extend `__attribute__((hot, optimize("O3,unroll-loops")))` (already used for numpy) to DSA graph ops and sorting
- DSA bulk operations use SIMD directly for distance matrix fill and sorting

---

### Phase 4 — Inlining + LTO + PGO *(planned)*
**Expected result: ≈1.0–1.05× C++ in hot paths**

- Auto-inline small LP functions (≤5 statements) called in hot loops
- Full `-flto=auto` for cross-TU inlining in release builds
- `lp build --pgo`: profile-guided optimization for judge-specific workloads

---

### Expected performance by workload

| Workload | Current | After Ph.1 | After Ph.2 | After Ph.3 | After Ph.4 |
|----------|---------|-----------|-----------|-----------|-----------|
| Numeric (DP, TSP) | 2.0× | 1.3× | 1.15× | 1.05× | 1.02× |
| Graph (BFS, Dijkstra) | 1.5× | 1.2× | 1.1× | 1.05× | 1.02× |
| String processing | 1.8× | 1.6× | 1.5× | 1.45× | 1.4× |
| I/O heavy (dsa) | 1.1× | 1.05× | 1.05× | 1.03× | 1.02× |
| Sorting | 2.5× | 1.4× | 1.2× | 1.05× | 1.02× |
| Mixed CP | 2.0× | 1.4× | 1.25× | 1.1× | 1.05× |

*Numbers are estimates based on overhead analysis. Re-run benchmarks on your build to confirm.*

## Benchmarks

### DP Problem (Knapsack variant)

| Implementation | Time | Memory |
|----------------|------|--------|
| C++ (vector) | 0.15s | 8 MB |
| LP (list) | 0.32s | 16 MB |
| Python (list) | 2.5s | 24 MB |

### Graph Algorithm (BFS)

| Implementation | Time (1M edges) |
|----------------|-----------------|
| C++ | 0.08s |
| LP (DSA graph) | 0.12s |
| Python | 0.8s |

### Sorting (1M integers)

| Implementation | Time |
|----------------|------|
| C++ (std::sort) | 0.1s |
| LP (list.sort()) | 0.25s |
| Python (sorted()) | 0.4s |

## Conclusion

LP provides a good balance between:
- **Developer productivity**: Python-like syntax
- **Performance**: ~2x slower than C++, much faster than Python
- **Safety**: Type checking, bounds checking

For competitive programming:
1. Use explicit type annotations
2. Use DSA module data structures
3. Pre-allocate arrays
4. Follow the optimization strategies above

With these techniques, LP can solve most competitive programming problems within time limits (usually 1-2 seconds).
