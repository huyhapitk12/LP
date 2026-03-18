# Runtime Modules

## What you'll learn

This guide documents the public runtime surface that the current LP compiler knows how to lower to native code, including verified modules, builtins that map directly into the runtime, and the most important limitations.

## Prerequisites

- Read [Language Basics](Language-Basics) first.
- Read [Expressions and Collections](Expressions-and-Collections) if you want more background on strings and collection access.

## Module Status Overview

| Module or surface | Status | Notes |
| --- | --- | --- |
| `math` | Supported | Direct mapping to native math helpers. |
| `random` | Supported | Seed, random, randint, uniform. |
| `time` | Supported | `time()` and `sleep()`. |
| `os` and `os.path` | Supported | Native file-system helpers. |
| `sys` | Supported | Platform string, argv helpers, exit, size helpers. |
| `http` | Supported | `get`, `post`, `put`, `delete`, `patch`. |
| `json` | Supported | `loads`, `dumps`, `parse`. |
| `sqlite` | Supported | `connect`, `execute`, `query`. |
| `thread` | Partially supported | Public surface is available, but `spawn` has strict compiler rules. |
| `memory` | Supported | Arena, pool, and `cast`. |
| `platform` | Supported | `os`, `arch`, `cores`. |
| `open()` and file handles | Supported | File-oriented runtime helpers. |
| `numpy` | Supported | Comprehensive array operations (see NumPy section). |
| `dsa` | Supported | Data Structures & Algorithms for competitive programming (see DSA section). |
| arbitrary Python import fallback | Internal or experimental | Exists in source, but current build path is not a stable public workflow yet. |

## `math`

### Supported API

- constants: `math.pi`, `math.e`, `math.tau`, `math.inf`, `math.nan_v`
- functions: `sqrt`, `fabs`, `ceil`, `floor`, `round`, `trunc`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`, `exp`, `log`, `log2`, `log10`, `pow`, `factorial`, `gcd`, `lcm`, `isnan`, `isinf`, `radians`, `degrees`

### Example

```lp
import math

def main():
    print(math.sin(1.0))
    print(math.gcd(12, 18))

main()
```

## `random`

### Supported API

- `random.seed(n)`
- `random.random()`
- `random.randint(a, b)`
- `random.uniform(a, b)`

### Example

```lp
import random

def main():
    random.seed(7)
    print(random.randint(1, 10))

main()
```

## `time`

### Supported API

- `time.time()`
- `time.sleep(seconds)`

### Example

```lp
import time

def main():
    start = time.time()
    time.sleep(0.1)
    print(time.time() - start)

main()
```

## `os` and `os.path`

### Supported API

- `os.getcwd()`
- `os.remove(path)`
- `os.rename(src, dst)`
- `os.mkdir(path)`
- `os.sep`
- `os.name`
- `os.path.join(a, b)`
- `os.path.exists(path)`
- `os.path.isfile(path)`
- `os.path.isdir(path)`
- `os.path.basename(path)`
- `os.path.dirname(path)`
- `os.path.getsize(path)`

### Example

```lp
import os

def main():
    path = os.path.join("logs", "app.txt")
    print(path)
    print(os.getcwd())

main()
```

## `sys`

### Supported API

- `sys.platform`
- `sys.maxsize`
- `sys.exit(code)`
- `sys.getrecursionlimit()`

### Example

```lp
import sys

def main():
    print(sys.platform)
    print(sys.maxsize)

main()
```

### Limitation

`sys.argv_*` and the size-helper internals exist in runtime source, but they are not documented here as stable public program-level APIs because the generated `main` path does not currently expose them as a verified end-to-end workflow.

## `http`

### Supported API

- `http.get(url)` - Perform HTTP GET request
- `http.post(url, data)` - Perform HTTP POST request
- `http.put(url, data)` - Perform HTTP PUT request
- `http.delete(url)` - Perform HTTP DELETE request
- `http.patch(url, data)` - Perform HTTP PATCH request

### Example

```lp
import http

def main():
    # GET request
    body: str = http.get("https://jsonplaceholder.typicode.com/posts/1")
    print(body)
    
    # POST request
    response = http.post("https://jsonplaceholder.typicode.com/posts", "title=LP&body=Hello")
    print(response)
    
    # PUT request
    updated = http.put("https://jsonplaceholder.typicode.com/posts/1", "title=Updated")
    print(updated)
    
    # DELETE request
    http.delete("https://jsonplaceholder.typicode.com/posts/1")
    
    # PATCH request
    patched = http.patch("https://jsonplaceholder.typicode.com/posts/1", "title=Patched")
    print(patched)

main()
```

## `json`

### Supported API

- `json.loads(text)` - Parse JSON string to value
- `json.dumps(value)` - Serialize value to JSON string
- `json.parse(text)` - Alias for `loads`

### Example

```lp
import json

def main():
    data = json.loads('{"name":"LP","ok":true}')
    print(str(data["name"]))
    print(json.dumps(data))
    
    # Using parse (alias for loads)
    data2 = json.parse('{"value":42}')
    print(int(data2["value"]))

main()
```

### Limitation

`json.loads(...)` returns a generic runtime value, so nested access often needs explicit casts such as `int(...)`, `str(...)`, or `bool(...)`.

## `sqlite`

### Supported API

- `sqlite.connect(path)`
- `sqlite.execute(db, sql)`
- `sqlite.query(db, sql)`

### Example

```lp
import sqlite

def main():
    db: sqlite_db = sqlite.connect("system.db")
    sqlite.execute(db, "CREATE TABLE IF NOT EXISTS logs (n INTEGER);")
    sqlite.execute(db, "INSERT INTO logs (n) VALUES (1);")
    rows = sqlite.query(db, "SELECT COUNT(*) AS n FROM logs")
    print(int(rows[0]["n"]))

main()
```

## `thread`

### Supported API

- `thread.spawn(worker)`
- `thread.spawn(worker, arg)`
- `thread.join(thread_handle)`
- `thread.lock_init()`
- `thread.lock_acquire(lock)`
- `thread.lock_release(lock)`

### Example

```lp
import thread

def worker(n: int) -> int:
    return n + 1

def main():
    t: thread = thread.spawn(worker, 41)
    print(thread.join(t))

main()
```

### Limitation

The worker restrictions described in [Concurrency and Parallelism](Concurrency-and-Parallelism) are part of the supported public contract.

## `memory`

### Supported API

- `memory.arena_new(size)`
- `memory.arena_alloc(arena, bytes)`
- `memory.arena_reset(arena)`
- `memory.arena_free(arena)`
- `memory.pool_new(chunk_size, num_chunks)`
- `memory.pool_alloc(pool)`
- `memory.pool_free(pool, ptr)`
- `memory.pool_destroy(pool)`
- `memory.cast(ptr, Type)`

### Example

```lp
import memory

class Pair:
    left: int = 0
    right: int = 0

def main():
    arena = memory.arena_new(64)
    ptr_a = memory.arena_alloc(arena, 16)
    pair_a = memory.cast(ptr_a, Pair)
    pair_a.left = 4
    pair_a.right = 5
    print(pair_a.left + pair_a.right)
    memory.arena_free(arena)

main()
```

## `platform`

### Supported API

- `platform.os()`
- `platform.arch()`
- `platform.cores()`

### Example

```lp
import platform

def main():
    print(str(platform.os()))
    print(str(platform.arch()))
    print(platform.cores())

main()
```

## `numpy`

LP provides a comprehensive NumPy-like array module with SIMD-optimized C implementations.

### Supported API

**Array Creation:**
- `np.array(data)` - Create array from data
- `np.zeros(n)` - Array of zeros
- `np.ones(n)` - Array of ones
- `np.arange(start, stop, step)` - Range array
- `np.linspace(a, b, n)` - Linearly spaced array
- `np.eye(n)` / `np.identity(n)` - Identity matrix

**Reductions:**
- `np.sum(arr)`, `np.mean(arr)`, `np.min(arr)`, `np.max(arr)`
- `np.std(arr)`, `np.var(arr)`, `np.median(arr)`
- `np.argmax(arr)`, `np.argmin(arr)`, `np.len(arr)`

**Element-wise Operations:**
- `np.sqrt(arr)`, `np.abs(arr)`, `np.sin(arr)`, `np.cos(arr)`
- `np.exp(arr)`, `np.log(arr)`, `np.power(arr, p)`
- `np.clip(arr, min, max)`, `np.sort(arr)`, `np.reverse(arr)`
- `np.cumsum(arr)`

**Shape Operations:**
- `np.reshape(arr, rows, cols)` - Reshape to 2D
- `np.flatten(arr)` - Flatten to 1D
- `np.transpose(arr)` - Transpose
- `np.diag(arr)` - Diagonal

**Linear Algebra:**
- `np.dot(a, b)` - Dot product
- `np.matmul(a, b)` - Matrix multiplication

**Array Operations:**
- `np.add(a, b)`, `np.sub(a, b)`, `np.mul(a, b)`, `np.div(a, b)`
- `np.scale(arr, scalar)`, `np.take(arr, indices)`
- `np.count_greater(arr, val)`, `np.count_less(arr, val)`, `np.count_equal(arr, val)`

### Example

```lp
import numpy as np

def main():
    # Array creation
    arr = np.array([1, 2, 3, 4, 5])
    print(np.sum(arr))       # 15
    print(np.mean(arr))      # 3.0
    print(np.max(arr))       # 5
    
    # Linear algebra
    a = np.array([1, 2, 3])
    b = np.array([4, 5, 6])
    print(np.dot(a, b))      # 32
    
    # Matrix operations
    mat = np.reshape(arr, 5, 1)
    transposed = np.transpose(mat)
    
    # Element-wise
    print(np.sqrt(arr))
    print(np.sort(arr))

main()
```

### Performance Note
All NumPy functions are compiled with `__attribute__((hot, optimize("O3,unroll-loops")))`, enabling auto-vectorization on supported platforms.

## `dsa` (Data Structures & Algorithms)

LP provides a comprehensive DSA module optimized for competitive programming with native C implementations. All functions have O(1) or O(log n) complexity unless otherwise noted.

### Fast I/O

Critical for competitive programming where input/output speed matters:

```lp
import dsa

def main():
    # Fast input
    n = dsa.read_int()        # Read integer
    x = dsa.read_float()      # Read float
    s = dsa.read_str()        # Read word
    line = dsa.read_line()    # Read entire line
    
    # Fast output
    dsa.write_int(n)          # Write integer
    dsa.write_str("Hello")    # Write string
    dsa.writeln()             # Write newline
    dsa.write_int_ln(n)       # Write integer + newline
    dsa.write_str_ln(s)       # Write string + newline
    dsa.flush()               # Flush output buffer
```

### Number Theory

```lp
import dsa

def main():
    # Prime operations
    is_prime = dsa.is_prime(17)           # Check if prime
    primes = dsa.sieve(1000000)           # Sieve of Eratosthenes
    
    # Modular arithmetic
    result = dsa.mod_pow(2, 100, 1000000007)  # (2^100) mod 10^9+7
    inverse = dsa.mod_inverse(3, 1000000007)  # Modular inverse
    
    # GCD operations
    ext_gcd = dsa.extended_gcd(35, 15)    # Returns (gcd, x, y)
    factors = dsa.prime_factors(360)      # Prime factorization
    
    # Number theory functions
    phi = dsa.euler_phi(100)              # Euler's totient
    div_count = dsa.count_divisors(360)   # Count divisors
    div_sum = dsa.sum_divisors(360)       # Sum of divisors
```

### Stack, Queue, Deque

```lp
import dsa

def main():
    # Stack (LIFO)
    stack = dsa.stack_new(100)
    dsa.stack_push(stack, 10)
    dsa.stack_push(stack, 20)
    top = dsa.stack_top(stack)    # 20
    val = dsa.stack_pop(stack)    # 20
    empty = dsa.stack_is_empty(stack)
    
    # Queue (FIFO)
    queue = dsa.queue_new(100)
    dsa.queue_push(queue, 1)
    dsa.queue_push(queue, 2)
    front = dsa.queue_front(queue)  # 1
    val = dsa.queue_pop(queue)      # 1
    
    # Deque (Double-ended)
    deque = dsa.deque_new(100)
    dsa.deque_push_front(deque, 1)
    dsa.deque_push_back(deque, 2)
    front = dsa.deque_front(deque)      # 1
    back = dsa.deque_back(deque)        # 2
    dsa.deque_pop_front(deque)
    dsa.deque_pop_back(deque)
```

### DSU (Disjoint Set Union / Union-Find)

```lp
import dsa

def main():
    n = 10
    dsu = dsa.dsu_new(n)
    
    # Union operations
    dsa.dsu_union(dsu, 0, 1)
    dsa.dsu_union(dsu, 2, 3)
    dsa.dsu_union(dsu, 1, 3)    # Now 0,1,2,3 are connected
    
    # Query operations
    same = dsa.dsu_same(dsu, 0, 3)    # True
    root = dsa.dsu_find(dsu, 2)       # Returns root
    size = dsa.dsu_size(dsu, 0)       # 4
```

### Heap / Priority Queue

```lp
import dsa

def main():
    heap = dsa.heap_new(100)
    
    # Push with priority (min-heap)
    dsa.heap_push(heap, 5, 5)    # (value, priority)
    dsa.heap_push(heap, 3, 3)
    dsa.heap_push(heap, 7, 7)
    
    # Query
    top = dsa.heap_top(heap)          # 3 (smallest priority)
    empty = dsa.heap_is_empty(heap)
    
    # Pop
    dsa.heap_pop(heap)    # Removes 3
```

### Fenwick Tree (Binary Indexed Tree)

```lp
import dsa

def main():
    n = 100
    ft = dsa.fenwick_new(n)
    
    # Point update
    dsa.fenwick_add(ft, 5, 10)    # Add 10 at index 5
    
    # Prefix sum query
    sum_0_to_10 = dsa.fenwick_prefix_sum(ft, 10)
    
    # Range sum query
    sum_5_to_15 = dsa.fenwick_range_sum(ft, 5, 15)
```

### Segment Tree with Lazy Propagation

```lp
import dsa

def main():
    arr = [1, 2, 3, 4, 5, 6, 7, 8]
    n = 8
    
    # Create segment tree (operations: "sum", "min", "max")
    seg = dsa.segtree_new(arr, n, "sum")
    dsa.segtree_build(seg)
    
    # Range query
    total = dsa.segtree_query(seg, 0, 7)   # Sum of all elements
    partial = dsa.segtree_query(seg, 2, 5) # Sum of arr[2..5]
    
    # Range update (lazy propagation)
    dsa.segtree_update(seg, 1, 4, 10)      # Add 10 to arr[1..4]
```

### Graph Algorithms

```lp
import dsa

def main():
    n = 5
    g = dsa.graph_new(n, True)     # directed graph
    
    # Add edges
    dsa.graph_add_edge(g, 0, 1, 10)    # from, to, weight
    dsa.graph_add_edge(g, 1, 2, 5)
    dsa.graph_add_edge(g, 0, 2, 20)
    
    # BFS - returns distances array
    dist = dsa.graph_bfs(g, 0)
    print(dist[2])    # Distance from 0 to 2
    
    # DFS - returns discovery/finish times
    dfs_result = dsa.graph_dfs(g, 0)
    print(dfs_result.disc[1])    # Discovery time
    print(dfs_result.finish[1])  # Finish time
    
    # Dijkstra's shortest path
    shortest = dsa.graph_dijkstra(g, 0)
    print(shortest[2])    # Shortest distance to node 2
    
    # Floyd-Warshall (all-pairs shortest path)
    all_dist = dsa.graph_floyd_warshall(g)
    print(all_dist[0][2])    # Distance from 0 to 2
```

### String Algorithms

```lp
import dsa

def main():
    text = "ababcabcababc"
    pattern = "abc"
    
    # KMP pattern matching
    positions = dsa.kmp_search(text, pattern)
    count = positions[0]         # Number of matches
    first_match = positions[1]   # First position
    
    # Z-algorithm
    z = dsa.z_algorithm("aabcaabxaaz")
    # z[i] = longest substring starting at i that matches prefix
    
    # Rolling hash for string matching
    rh = dsa.rolling_hash_new(text, 31, 1000000007)
    hash_0_3 = dsa.rolling_hash_get(rh, 0, 3)   # Hash of text[0:3]
    hash_5_8 = dsa.rolling_hash_get(rh, 5, 8)   # Hash of text[5:8]
```

### Geometry

```lp
import dsa

def main():
    # Points
    a = dsa.point(0.0, 0.0)
    b = dsa.point(3.0, 4.0)
    
    # Distance
    d = dsa.dist(a, b)           # 5.0
    d_sq = dsa.dist_sq(a, b)     # 25.0
    
    # Cross product (for orientation)
    c = dsa.point(1.0, 1.0)
    cross = dsa.cross(a, b, c)   # (b-a) x (c-a)
    
    # Collinearity
    col = dsa.collinear(a, b, c)
    
    # Segment intersection
    d1 = dsa.point(0.0, 0.0)
    d2 = dsa.point(2.0, 2.0)
    d3 = dsa.point(0.0, 2.0)
    d4 = dsa.point(2.0, 0.0)
    intersect = dsa.segments_intersect(d1, d2, d3, d4)    # True
    
    # Convex hull
    points = [dsa.point(0, 0), dsa.point(1, 1), dsa.point(2, 0), dsa.point(1, -1)]
    hull = dsa.convex_hull(points, 4)
    print(hull.count)    # Number of hull points
    
    # Polygon area (shoelace formula)
    area = dsa.polygon_area(hull.points, hull.count)
    
    # Point in polygon
    inside = dsa.point_in_polygon(hull.points, hull.count, dsa.point(0.5, 0))
```

### Performance Characteristics

| Data Structure | Operation | Complexity |
|---------------|-----------|------------|
| Stack | push, pop, top | O(1) |
| Queue | push, pop, front | O(1) |
| Deque | all operations | O(1) |
| DSU | find, union, same | O(α(n)) ≈ O(1) |
| Heap | push, pop, top | O(log n) |
| Fenwick Tree | add, prefix_sum, range_sum | O(log n) |
| Segment Tree | build, update, query | O(n), O(log n), O(log n) |
| Graph BFS/DFS | traversal | O(V + E) |
| Dijkstra | shortest path | O((V + E) log V) |
| Floyd-Warshall | all-pairs | O(V³) |
| KMP | pattern search | O(n + m) |
| Z-algorithm | all matches | O(n) |
| Convex Hull | Andrew's algorithm | O(n log n) |

## File I/O builtins

### Supported API

- `open(path, mode)`
- `file.read()`
- `file.write(text)`
- `file.close()`
- file handles inside `with`

### Example

```lp
def main():
    with open("notes.txt", "w") as f:
        f.write("LP\n")

    with open("notes.txt", "r") as f:
        print(f.read())

main()
```

## Value conversion builtins

Common builtins used across the examples and tests:

- `str(x)`
- `int(x)`
- `float(x)`
- `bool(x)`
- `len(x)`
- `print(x)`
- `range(...)`

These are part of the everyday LP surface even though they are not imported from modules.

## Common errors and limitations

- `thread.spawn(...)` is not a generic callback runner. It requires a named LP worker function with 0-1 arguments returning int or void.
- `memory.cast(...)` expects a class type in the documented public workflow.
- `os.path.*` is a stable native path for file-system helpers. Arbitrary nested module calls outside documented cases are not guaranteed.
- Arbitrary Python imports exist in the compiler source, but the build path is still environment-sensitive and not documented as stable.

## See also

- [Concurrency and Parallelism](Concurrency-and-Parallelism)
- [CLI and Tooling](CLI-and-Tooling)
- [Language Reference](Language-Reference)
