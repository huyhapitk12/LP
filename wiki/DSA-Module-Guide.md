# DSA Module Guide

The DSA (Data Structures & Algorithms) module provides optimized tools for Competitive Programming.

## 🚀 Fast I/O - Fast Input/Output

### Import

```lp
import dsa
```

### Input Functions

| Function | Description | Example |
|----------|-------------|---------|
| `dsa.read_int()` | Read integer | `n: int = dsa.read_int()` |
| `dsa.read_float()` | Read float | `x: float = dsa.read_float()` |
| `dsa.read_str()` | Read one word | `s: str = dsa.read_str()` |
| `dsa.read_line()` | Read one line | `line: str = dsa.read_line()` |

### Output Functions

| Function | Description | Example |
|----------|-------------|---------|
| `dsa.write_int(x)` | Write integer | `dsa.write_int(123)` |
| `dsa.write_int_ln(x)` | Write integer + newline | `dsa.write_int_ln(ans)` |
| `dsa.write_str(s)` | Write string | `dsa.write_str("Hello")` |
| `dsa.write_str_ln(s)` | Write string + newline | `dsa.write_str_ln("Done")` |
| `dsa.write_char(c)` | Write character | `dsa.write_char('A')` |
| `dsa.write_float(x)` | Write float (4 decimal places) | `dsa.write_float(3.14159)` |
| `dsa.write_float_prec(x, d)` | Write float with `d` decimal places | `dsa.write_float_prec(best, 4)` |
| `dsa.write_float_prec_ln(x, d)` | Write float with precision + newline | `dsa.write_float_prec_ln(ans, 6)` |
| `dsa.writeln()` | Write newline | `dsa.writeln()` |
| **`dsa.flush()`** | **Flush buffer to stdout** | `dsa.flush()` |

### ⚠️ IMPORTANT: Always call `dsa.flush()`

```lp
import dsa

def main():
    n: int = dsa.read_int()
    dsa.write_int_ln(n * 2)
    dsa.flush()  # REQUIRED - flush output before exit

main()
```

## 📝 Complete Examples

### Read Array and Calculate Sum

```lp
import dsa

def main():
    # Read n integers
    n: int = dsa.read_int()
    
    arr: list = []
    total: int = 0
    
    for i in range(n):
        val: int = dsa.read_int()
        arr.append(val)
        total = total + val
    
    dsa.write_str("Sum: ")
    dsa.write_int_ln(total)
    dsa.flush()

main()
```

### Read Matrix

```lp
import dsa

def main():
    n: int = dsa.read_int()
    m: int = dsa.read_int()
    
    # Create 1D array simulating 2D
    row_size: int = m + 2
    mat: list = [0] * ((n + 2) * row_size)
    
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            idx: int = i * row_size + j
            mat[idx] = dsa.read_int()
    
    # Output sum
    total: int = 0
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            total = total + mat[i * row_size + j]
    
    dsa.write_int_ln(total)
    dsa.flush()

main()
```

### Input from File

```bash
# Create input file
echo "3
1 2 3" > input.txt

# Run with input from file
lp solution.lp < input.txt

# Or compile then run
lp solution.lp -c solution
./solution < input.txt
```

## 🔧 Data Structures

### Stack

```lp
import dsa

def main():
    stack = dsa.stack_new(100)
    
    dsa.stack_push(stack, 10)
    dsa.stack_push(stack, 20)
    
    top: int = dsa.stack_top(stack)  # 20
    val: int = dsa.stack_pop(stack)  # 20
    
    empty: bool = dsa.stack_is_empty(stack)
    
    dsa.stack_free(stack)
    dsa.flush()
```

### Queue

```lp
import dsa

def main():
    q = dsa.queue_new(100)
    
    dsa.queue_push(q, 1)
    dsa.queue_push(q, 2)
    
    front: int = dsa.queue_front(q)  # 1
    val: int = dsa.queue_pop(q)      # 1
    
    dsa.queue_free(q)
    dsa.flush()
```

### DSU (Disjoint Set Union)

```lp
import dsa

def main():
    n: int = dsa.read_int()
    dsu = dsa.dsu_new(n)
    
    # Union
    dsa.dsu_union(dsu, 1, 2)
    dsa.dsu_union(dsu, 2, 3)
    
    # Find
    root: int = dsa.dsu_find(dsu, 1)
    
    # Check same set
    same: bool = dsa.dsu_same(dsu, 1, 3)  # True
    
    dsa.dsu_free(dsu)
    dsa.flush()
```

### Heap (Priority Queue)

```lp
import dsa

def main():
    heap = dsa.heap_new(100)
    
    # Push (value, priority)
    dsa.heap_push(heap, 5, 2)   # value=5, priority=2
    dsa.heap_push(heap, 3, 1)   # value=3, priority=1 (min heap)
    
    top_val: int = dsa.heap_top(heap)           # 3
    top_pri: int = dsa.heap_top_priority(heap)  # 1
    
    dsa.heap_pop(heap)
    dsa.heap_free(heap)
    dsa.flush()
```

### Fenwick Tree (BIT)

```lp
import dsa

def main():
    n: int = 10
    bit = dsa.fenwick_new(n)
    
    # Add value at index
    dsa.fenwick_add(bit, 0, 5)   # arr[0] += 5
    dsa.fenwick_add(bit, 3, 10)  # arr[3] += 10
    
    # Prefix sum [0..idx]
    sum_val: int = dsa.fenwick_prefix_sum(bit, 3)
    
    # Range sum [l..r]
    range_sum: int = dsa.fenwick_range_sum(bit, 0, 5)
    
    dsa.fenwick_free(bit)
    dsa.flush()
```

## 📐 Graph Algorithms

### BFS

```lp
import dsa

def main():
    n: int = 5
    g = dsa.graph_new(n, False)  # undirected
    
    dsa.graph_add_edge(g, 0, 1, 1)  # u, v, weight
    dsa.graph_add_edge(g, 1, 2, 1)
    
    dist = dsa.graph_bfs(g, 0)  # distances from node 0
    
    dsa.graph_free(g)
    dsa.flush()
```

### Dijkstra

```lp
import dsa

def main():
    n: int = 5
    g = dsa.graph_new(n, True)  # directed
    
    dsa.graph_add_edge(g, 0, 1, 10)  # u, v, weight
    dsa.graph_add_edge(g, 1, 2, 5)
    
    dist = dsa.graph_dijkstra(g, 0)  # shortest paths from 0
    
    dsa.graph_free(g)
    dsa.flush()
```

## 🔢 Number Theory

```lp
import dsa

def main():
    # GCD
    g: int = dsa.gcd(12, 8)  # 4
    
    # LCM
    l: int = dsa.lcm(4, 6)   # 12
    
    # Mod power: (base^exp) % mod
    p: int = dsa.mod_pow(2, 10, 1000)  # 1024 % 1000 = 24
    
    # Is prime
    prime: bool = dsa.is_prime(17)  # True
    
    # Sieve
    is_prime_arr = dsa.sieve(100)
    
    dsa.flush()
```

## 🎯 String Algorithms

### KMP

```lp
import dsa

def main():
    text: str = "ababcabcabababd"
    pattern: str = "ababd"
    
    positions = dsa.kmp_search(text, pattern)
    # positions[0] = count
    # positions[1..] = positions found
    
    dsa.flush()
```

### Z-Algorithm

```lp
import dsa

def main():
    s: str = "aabcaabxaaz"
    z_arr = dsa.z_algorithm(s)
    # z_arr[i] = longest prefix match starting at i
    
    dsa.flush()
```

## ⚡ Performance

| Operation | Complexity |
|-----------|------------|
| read_int/write_int | O(1) amortized |
| Stack push/pop | O(1) |
| Queue push/pop | O(1) |
| DSU find/union | O(α(n)) ~ O(1) |
| Heap push/pop | O(log n) |
| Fenwick add/query | O(log n) |
| BFS | O(V + E) |
| Dijkstra | O((V + E) log V) |

## 🚀 Performance Tips for Competitive Programming

### LP vs C++ Performance

LP is typically **~2x slower** than C++ but much faster than Python:

| Language | Typical Time |
|----------|--------------|
| C++ | 0.15s (baseline) |
| LP | ~0.30s (~2x) |
| Python | ~2-5s (15-30x) |

### Why LP is Slower

1. **Tagged Union (LpVal)**: Every value has type information (16 bytes vs 8 bytes for raw int)
2. **Dynamic Arrays**: LpList has bounds checking and indirection
3. **Type Checks**: Every operation checks types before executing
4. **No SIMD**: Cannot use CPU vectorization

### Optimization Strategies

#### 1. Pre-allocate Arrays
```lp
# SLOW - Dynamic growth
arr: list = []
for i in range(n):
    arr.append(dsa.read_int())

# FAST - Pre-allocated
arr: list = [0] * n
for i in range(n):
    arr[i] = dsa.read_int()
```

#### 2. Cache Index Calculations
```lp
# SLOW - Recalculate index multiple times
for i in range(n):
    for j in range(m):
        mat[i * row_size + j] = mat[i * row_size + j] + 1

# FAST - Cache the index
for i in range(n):
    row_start: int = i * row_size
    for j in range(m):
        idx: int = row_start + j
        mat[idx] = mat[idx] + 1
```

#### 3. Use DSA Data Structures
```lp
# DSA structures are optimized for their purpose
stack = dsa.stack_new(10000)  # Faster than list append/pop
queue = dsa.queue_new(10000)  # Faster than list for FIFO
```

#### 4. Use Explicit Types
```lp
# Good - Compiler knows types
n: int = dsa.read_int()
sum: int = 0
for i in range(n):
    sum = sum + dsa.read_int()

# Avoid - Dynamic types have overhead
data = []  # No type hint
```

### When LP Might TLE

If your solution is near the time limit:
1. **Try optimizing the algorithm first** - O(n log n) is usually fine
2. **Use DSA module structures** - They're optimized
3. **Consider rewriting critical sections** - Sometimes a different approach helps
4. **Check for unnecessary list operations** - Reduce indirection

See [Performance Guide](Performance-Guide) for detailed analysis.

## ❌ Common Errors

### 1. Missing `dsa.flush()`

```lp
# WRONG - No output
def main():
    n: int = dsa.read_int()
    dsa.write_int_ln(n * 2)
    # Missing flush!

# CORRECT
def main():
    n: int = dsa.read_int()
    dsa.write_int_ln(n * 2)
    dsa.flush()  # Required!
```

### 2. Type confusion

```lp
# VALID - low-level pointer usage
arena: ptr = None

# PREFERRED - use list for arrays
arr: list = [0] * 100
```

### 3. Uninitialized array

```lp
# WRONG
arr: list  # Not initialized

# CORRECT
arr: list = [0] * 100
```

### 4. Using None instead of null

```lp
# WRONG
x: int = None

# CORRECT (if needed)
x: int = 0
# Or use null for pointers
```
