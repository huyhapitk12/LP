# LP Language Feature Status

> Last updated: 2025-03-14
> Version: 0.3.0

This document provides a comprehensive overview of LP language features, their implementation status, and any known limitations.

## Quick Reference

| Category | Fully Supported | Partial | Not Yet |
|----------|----------------|---------|---------|
| Core Syntax | 25+ | 0 | 0 |
| Runtime Modules | 12 | 0 | 0 |
| Operators | 20+ | 0 | 0 |
| CLI Tools | 10 | 0 | 0 |
| Optimizations | 3 | 0 | 0 |
| Modern Features | 6 | 0 | 0 |

## Compilation Backends

### ✅ Native Assembly Backend (Default)

LP compiles directly to x86-64 assembly without requiring GCC or LLVM:

```bash
lp file.lp              # Native compilation (default)
lp file.lp --gcc        # Use GCC backend (optional)
```

**Benefits:**
- No heavy compiler dependencies (~5MB vs ~1GB)
- Fast compilation
- Direct control over generated code

### ✅ GCC C Backend (Optional)

For maximum compatibility and optimization:

```bash
lp file.lp --gcc        # Use GCC backend
lp file.lp -o out.c     # Generate C code
```

## Optimizations

### ✅ Constant Folding

Evaluate constant expressions at compile time:

```lp
# Before optimization
x: int = 1 + 2 * 3
y: int = 100 / 5

# After optimization (compile-time)
x: int = 7
y: int = 20
```

### ✅ Dead Code Elimination

Remove unreachable code:

```lp
if False:
    # This code is removed
    print("Never executed")
```

### ✅ Loop Unrolling

Unroll small loops for better performance:

```lp
# Small constant loops are unrolled
for i in range(4):
    print(i)
```

---

## Core Language Features

### ✅ Fully Supported

#### Variables and Types

```lp
# Type inference
x = 42              # int
pi = 3.14           # float
name = "LP"         # str
flag = True         # bool

# Explicit types
count: int = 100
ratio: float = 0.5
label: str = "text"
active: bool = False
```

#### Functions

```lp
# Basic function
def add(a: int, b: int) -> int:
    return a + b

# Default arguments
def greet(name: str = "World") -> str:
    return "Hello " + name

# Variadic functions
def sum_all(*args) -> int:
    total = 0
    for i in args:
        total = total + i
    return total

# Keyword arguments (**kwargs)
def configure(**kwargs):
    pass
```

#### Classes and OOP

```lp
# Base class
class Entity:
    x: float = 0.0
    y: float = 0.0
    
    def move(self, dx: float, dy: float):
        self.x = self.x + dx
        self.y = self.y + dy

# Inheritance
class Player(Entity):
    name: str = "Hero"
    health: int = 100

# Access modifiers
class Secret:
    private hidden: int = 42
    protected internal: str = "internal"
    public visible: bool = True
```

#### Control Flow

```lp
# Conditionals
if score >= 90:
    grade = "A"
elif score >= 80:
    grade = "B"
else:
    grade = "C"

# For loops
for i in range(10):
    print(i)

for i in range(0, 100, 5):
    print(i)

# While loops
while count > 0:
    count -= 1
    if count == 5:
        continue
    if count == 2:
        break

# Pass statement
def placeholder():
    pass
```

#### Exception Handling

```lp
# Try-except-finally
try:
    result = risky_operation()
except:
    print("Error occurred")
finally:
    cleanup()

# Raise exception
def validate(x: int):
    if x < 0:
        raise "x must be non-negative"
```

#### Context Managers

```lp
# File handling
with open("data.txt", "r") as f:
    content = f.read()
    print(content)
```

#### Lambda Functions

```lp
# Single-line lambda
add = lambda x, y: x + y
result = add(3, 5)

# Multiline lambda
process = lambda x:
    y = x * 2
    z = y + 1
    return z
```

#### Collections

```lp
# List
numbers = [1, 2, 3, 4, 5]
numbers.append(6)
print(numbers[0])

# Dict
config = {"host": "localhost", "port": 8080}
config["debug"] = True

# Set
unique = {1, 2, 3, 2, 1}  # {1, 2, 3}

# Tuple
point = (10, 20)
x = point[0]
```

#### String Operations

```lp
s = "  Hello World  "

# Case conversion
s.upper()           # "  HELLO WORLD  "
s.lower()           # "  hello world  "

# Whitespace
s.strip()           # "Hello World"
s.lstrip()          # "Hello World  "
s.rstrip()          # "  Hello World"

# Search and replace
s.find("World")     # 8
s.replace("World", "LP")  # "  Hello LP  "
s.count("l")        # 3

# Split and join
parts = "a,b,c".split(",")  # ["a", "b", "c"]
joined = "-".join(parts)     # "a-b-c"

# Checks
"123".isdigit()     # True
"abc".isalpha()     # True
"a1b2".isalnum()    # True
s.startswith("  H") # True
s.endswith("d  ")   # True
```

#### Operators

**Arithmetic:**
```lp
a + b       # Addition
a - b       # Subtraction
a * b       # Multiplication
a / b       # Division (float)
a // b      # Floor division
a % b       # Modulo (Python-style)
a ** b      # Power
```

**Bitwise:**
```lp
a & b       # AND
a | b       # OR
a ^ b       # XOR
~a          # NOT
a << n      # Left shift
a >> n      # Right shift
```

**Comparison:**
```lp
a == b      # Equal
a != b      # Not equal
a < b       # Less than
a > b       # Greater than
a <= b      # Less or equal
a >= b      # Greater or equal
```

**Logical:**
```lp
a and b     # Logical AND
a or b      # Logical OR
not a       # Logical NOT
a is b      # Identity
a in list   # Membership
```

**Assignment:**
```lp
a = b
a += b
a -= b
a *= b
a /= b
a &= b
a |= b
a ^= b
a <<= n
a >>= n
```

#### Parallel Execution

```lp
# Parallel for loop (OpenMP is automatically enabled)
parallel for i in range(1000000):
    process_item(i)

# Parallel for with range parameters
parallel for i in range(0, 100, 5):
    print(i)
```

**Generated C Code:**
```c
#pragma omp parallel for
for (int64_t lp_i = 0; lp_i < 1000000; lp_i++) {
    process_item(lp_i);
}
```

**Compilation:**
```bash
# OpenMP is automatically detected and enabled
lp file.lp              # Run directly
lp file.lp -c output    # Compile to executable
lp file.lp -o file.c    # Generate C only
```

**Note:** The LP compiler automatically adds `-fopenmp` when it detects `parallel for` loops. No manual flags needed!

#### @settings Decorator

The `@settings` decorator allows fine-grained control over parallel and GPU execution:

```lp
# Configure thread count
@settings(threads=8)
def parallel_process(data: list) -> list:
    results = []
    parallel for item in data:
        results.append(process(item))
    return results

# Use GPU acceleration
@settings(device="gpu", gpu_id=0)
def gpu_compute(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i * i
    return result

# Dynamic scheduling with chunk size
@settings(threads=4, schedule="dynamic", chunk=100)
def process_large_dataset(data: list) -> int:
    count = 0
    parallel for item in data:
        count += item
    return count

# Auto-select best device (GPU if available, otherwise CPU)
@settings(device="auto")
def auto_parallel(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i
    return result
```

**Available Settings:**
| Option | Description | Example Values |
|--------|-------------|----------------|
| `threads` | Number of threads for parallel execution | `4`, `8`, `16` |
| `schedule` | OpenMP scheduling policy | `"static"`, `"dynamic"`, `"guided"`, `"auto"` |
| `chunk` | Chunk size for scheduling | `100`, `1000` |
| `device` | Execution device | `"cpu"`, `"gpu"`, `"auto"` |
| `gpu_id` | GPU device ID | `0`, `1` |
| `unified` | Use unified memory for GPU | `True`, `False` |

**Scheduling Policies:**
- `static`: Divide work evenly (default)
- `dynamic`: Work in chunks, good for uneven workloads
- `guided`: Decreasing chunk sizes
- `auto`: Let runtime decide

**Generated C Code:**
```c
// @settings(threads=4, schedule="dynamic", chunk=100)
lp_parallel_set_threads(4);
lp_parallel_configure(4, "dynamic", 100, 0, 0);
#pragma omp parallel for num_threads(4) schedule(dynamic, 100)
for (int64_t lp_i = 0; lp_i < n; lp_i++) {
    // ...
}
```

---

### ⚠️ Partially Supported

#### List Comprehensions

**Status:** Numeric-focused, limited functionality

```lp
# Works
squares = [i * i for i in range(10)]

# Limited - may not work for complex expressions
filtered = [x for x in data if x > 0]  # Partial support
```

#### Threading

**Status:** Strict worker restrictions

```lp
# ✅ Allowed - named function, 0 or 1 arg, returns int/void
def worker(x: int) -> int:
    return x * 2

t = thread.spawn(worker, 42)
result = thread.join(t)

# ❌ NOT Allowed
t = thread.spawn(lambda x: x * 2, 42)  # Lambda not allowed
t = thread.spawn(worker, 1, 2)         # Too many args
def bad() -> str: return "hi"          # Wrong return type
```

#### NumPy Module

**Status:** Basic functions only

```lp
import numpy as np

# ✅ Supported
arr = np.array([1, 2, 3])
zeros = np.zeros(10)
ones = np.ones(5)
ranged = np.arange(0, 100, 5)
spaced = np.linspace(0, 1, 50)
squared = np.sqrt(arr)
absolute = np.abs(arr)
sorted_arr = np.sort(arr)

# ❌ Not available
# np.dot(), np.matmul(), np.reshape(), etc.
```

---

### ✅ Newly Added Features (v0.1.1)

These features were recently added and are now fully supported:

#### HTTP POST

```lp
# ✅ Now supported!
response = http.post("https://api.example.com/submit", "data=value")

# ✅ Works - GET requests
response = http.get("https://api.example.com/data")
```

**Note:** Both `http.get()` and `http.post()` are now fully supported.

#### JSON Parse

```lp
# ✅ All supported
data = json.loads('{"name": "LP"}')   # Parse JSON string
data = json.parse('{"name": "LP"}')   # Alias for loads
text = json.dumps(data)                # Serialize to JSON
```

#### Dictionary Comprehensions

```lp
# ✅ Now supported!
squares = {x: x*x for x in range(10)}

# With condition
evens = {x: x*2 for x in range(10) if x % 2 == 0}
```

#### Generators / Yield

```lp
# ✅ Now supported (basic generator pattern)
def counter(n):
    i = 0
    while i < n:
        yield i
        i += 1
```

---

### ❌ Not Supported Yet

*No features currently marked as not supported. All planned features are now implemented!*

---

### ✅ Newly Added Features (v0.3.0)

These features were recently added and are now fully supported:

#### F-Strings

```lp
# ✅ Now supported!
name = "LP"
version = 1
greeting = f"Hello from {name} v{version}!"
print(greeting)  # Output: Hello from LP v1!

# Expressions in f-strings
x = 10
y = 20
print(f"Sum: {x + y}")  # Output: Sum: 30
```

#### Pattern Matching (match/case)

```lp
# ✅ Now supported!
match value:
    case 1:
        print("one")
    case 2:
        print("two")
    case _:
        print("other")

# With guard expressions
match x:
    case n if n > 0:
        print("positive")
    case _:
        print("non-positive")
```

#### Decorators

```lp
# ✅ Now supported!
@settings(threads=4)
def parallel_task():
    print("Running with 4 threads")

# Multiple decorators
@decorator1
@decorator2
def func():
    pass

# ✅ NEW: Security decorator for input validation and access control
@security
def basic_secure():
    print("Function with default security")

@security(level="high", auth="bearer")
def authenticated_api():
    print("API requiring authentication")

@security(rate_limit=100, validate=True, injection=True)
def safe_input_handler():
    print("Protected against injection attacks")

@security(admin, readonly)
def admin_readonly():
    print("Admin access, readonly mode")
```

#### @security Decorator Options

The `@security` decorator provides comprehensive security features for functions:

| Option | Type | Description |
|--------|------|-------------|
| `level` | int/str | Security level: 0-4 or "none", "low", "medium", "high", "critical" |
| `auth` | str | Authentication type: "basic", "bearer", "api_key", "oauth" |
| `rate_limit` | int | Maximum requests per minute |
| `validate` | bool | Enable input validation |
| `sanitize` | bool | Enable output sanitization |
| `injection` | bool | Prevent SQL/command injection |
| `xss` | bool | Prevent XSS attacks |
| `csrf` | bool | Enable CSRF protection |
| `cors` | str/bool | Enable CORS with specified origins |
| `headers` | bool | Enable secure headers |
| `encrypt` | bool | Enable data encryption |
| `hash` | str | Hash algorithm: "md5", "sha256", "sha512" |
| `readonly` | flag | Enable readonly mode |
| `admin` | flag | Set access level to admin |
| `user` | flag | Set access level to user |
| `guest` | flag | Set access level to guest |

#### Async/Await

```lp
# ✅ Now supported!
async def fetch_data(url: str) -> str:
    return "data from " + url

async def main():
    data = await fetch_data("https://example.com")
    print(data)
```

**Note:** Async/await is implemented as syntactic sugar for synchronous code. Great for code organization and future extensibility.

#### Type Unions

```lp
# ✅ Now supported!
def process(x: int | str) -> int | str:
    return x

# Variables with union types
data: int | str | float = 42
data = "hello"
data = 3.14
```

**Note:** Union types are represented as `LpVal` (variant type) at runtime with automatic type tagging.

#### Generic Types

```lp
# ✅ Now supported!
class Box[T]:
    value: T
    
    def get(self) -> T:
        return self.value
    
    def set(self, val: T):
        self.value = val

# Usage
int_box: Box[int] = Box()
int_box.set(42)
value = int_box.get()

# Multiple type parameters
class Pair[K, V]:
    key: K
    value: V
```

---

## Runtime Modules

### ✅ Fully Supported

| Module | Functions | Constants |
|--------|-----------|-----------|
| `math` | sin, cos, tan, sqrt, pow, factorial, gcd, lcm, isnan, isinf, floor, ceil, round, abs, log, exp | pi, e, tau |
| `random` | seed, random, randint, uniform | - |
| `time` | time, sleep | - |
| `os` | getcwd, remove, rename, mkdir | sep, name |
| `os.path` | join, exists, isfile, isdir, basename, dirname, getsize | - |
| `sys` | platform, maxsize, exit, getrecursionlimit, argv_len, argv_get | platform, maxsize |
| `sqlite` | connect, execute, query | - |
| `memory` | arena_new, arena_alloc, arena_reset, arena_free, pool_new, pool_alloc, pool_free, pool_destroy, cast | - |
| `platform` | os, arch, cores | - |

### ⚠️ Partially Supported

| Module | Supported | Not Supported |
|--------|-----------|---------------|
| `http` | `get` | `post`, `put`, `delete` |
| `json` | `loads`, `dumps` | `parse` |
| `numpy` | array, zeros, ones, arange, linspace, sqrt, abs, sin, cos, sort | dot, matmul, reshape, etc. |
| `thread` | spawn, join, lock_* | (worker restrictions apply) |

---

## CLI Commands

### ✅ All Supported

```bash
# Run directly (native ASM backend - default, no GCC!)
lp file.lp

# Run with GCC backend (optional)
lp file.lp --gcc

# Generate C code
lp file.lp -o output.c

# Compile executable
lp file.lp -c output.exe

# Generate assembly
lp file.lp -asm output.s

# Run tests
lp test examples

# Profile execution
lp profile file.lp

# Watch mode (recompile on change)
lp watch file.lp

# Build with flags
lp build file.lp --release --strip

# Package distribution
lp package file.lp --format zip

# Export C API
lp export file.lp -o api_name
lp export file.lp --library -o mylib
```

---

## Cross-Platform Support

| Platform | Architecture | Status |
|----------|--------------|--------|
| Linux | x64 | ✅ Full |
| Linux | ARM64 | ✅ Full |
| Windows | x64 | ✅ Full (MSYS2 UCRT64) |
| macOS | ARM64 | ✅ Full |
| macOS | x64 | ⚠️ Untested |

---

## Type System

### Supported Types

| Type | LP Syntax | C Equivalent |
|------|-----------|--------------|
| Integer | `int`, `i64` | `int64_t` |
| Float | `float`, `f64` | `double` |
| String | `str` | `const char*` |
| Boolean | `bool` | `int` |
| Void | `void` | `void` |
| List | `list` | `LpList*` |
| Dict | `dict` | `LpDict*` |
| Set | `set` | `LpSet*` |
| Tuple | `tuple` | `LpTuple*` |
| Dynamic value | `val` | `LpVal` |
| Raw pointer | `ptr` | `void*` |
| Memory arena | `arena` | `LpArena*` |
| Memory pool | `pool` | `LpPool*` |
| SQLite DB | `sqlite_db` | `sqlite3*` |
| Thread handle | `thread` | `LpThread` |
| Mutex lock | `lock` | `LpLock` |

---

## Performance Characteristics

| Feature | Performance |
|---------|-------------|
| Compiled code | Native C performance |
| Memory management | Manual + arena/pool allocators |
| Function calls | Direct C function calls |
| String operations | Zero-copy where possible |
| Collections | Hash table O(1) lookup |
| Parallel loops | OpenMP-based (when available) |

---

## Coming Soon (Roadmap)

1. **GPU Acceleration** - CUDA/OpenCL support via `@settings(device="gpu")`
2. **Parallel Settings** - Fine-grained control: `@settings(threads=8, schedule="dynamic")`
3. **Automatic Parallelization** - Compiler auto-detects parallelizable loops
4. **Distributed Computing** - Multi-node parallel execution
5. **Better error messages** - More descriptive compiler errors
6. **Package manager** - Dependency management
7. **Standard library expansion** - More modules and utilities

---

## Potential Future Features

The following features are planned or being considered for future releases:

### High Priority

| Feature | Description | Status |
|---------|-------------|--------|
| **GPU Computing** | CUDA/OpenCL kernel generation | Runtime ready, compiler integration planned |
| **Parallel Reductions** | Built-in parallel sum, min, max | Runtime ready |
| **Async/Await** | Asynchronous programming | Planned |
| **Pattern Matching** | `match/case` statements | Planned |
| **F-Strings** | Formatted string literals | Planned |
| **Decorators** | Function decorators | Planned |

### Medium Priority

| Feature | Description | Status |
|---------|-------------|--------|
| **Generic Types** | Type parameters for classes | Planned |
| **Type Unions** | `int \| str` type annotations | Planned |
| **Exception Types** | Custom exception classes | Planned |
| **Iterator Protocol** | `__iter__`, `__next__` methods | Planned |
| **Context Managers** | `__enter__`, `__exit__` methods | Planned |
| **Operator Overloading** | `__add__`, `__sub__`, etc. | Planned |

### Low Priority / Experimental

| Feature | Description | Status |
|---------|-------------|--------|
| **Metaclasses** | Advanced OOP features | Exploring |
| **Macros** | Compile-time code generation | Exploring |
| **Contracts** | Design by contract | Exploring |
| **Effect System** | Track side effects | Exploring |
| **Linear Types** | Resource management | Exploring |

### Parallel Computing Features

| Feature | Description | Status |
|---------|-------------|--------|
| **OpenMP Integration** | `parallel for` loops | ✅ Implemented |
| **Thread Management** | `parallel.threads()`, `parallel.cores()` | ✅ Runtime ready |
| **Parallel Algorithms** | Parallel sort, reduce, scan | Runtime ready |
| **GPU Detection** | `gpu.is_available()`, `gpu.device_name()` | ✅ Runtime ready |
| **GPU Memory** | Unified memory allocation | Runtime ready |
| **GPU Kernels** | Auto-generate CUDA/OpenCL | Planned |
| **Distributed Computing** | Multi-node execution | Exploring |

### Module Extensions

| Module | New Functions | Status |
|--------|--------------|--------|
| `http` | PUT, DELETE, PATCH, headers, auth | Planned |
| `json` | Schema validation, streaming | Planned |
| `sqlite` | Migrations, ORM-like features | Planned |
| `numpy` | Matrix operations, FFT, linear algebra | Planned |
| `thread` | Thread pools, futures, channels | Planned |
| `parallel` | Reductions, algorithms | Runtime ready |
| `gpu` | Memory management, kernels | Runtime ready |

---

## Performance Tips

### Parallel Computing

1. **Use appropriate iteration counts** - Parallel overhead is worth it for large loops
2. **Avoid data dependencies** - Each iteration should be independent
3. **Use dynamic scheduling** - For uneven workloads: `@settings(schedule="dynamic")`
4. **Set thread affinity** - Use `OMP_PROC_BIND=true` environment variable

### Memory Management

1. **Use arenas for temporary allocations** - Faster allocation/deallocation
2. **Prefer pools for fixed-size objects** - Better cache locality
3. **Minimize allocations in hot loops** - Allocate once, reuse

### String Operations

1. **Use `join()` for concatenation** - More efficient than `+` in loops
2. **Cache frequently used strings** - Avoid repeated allocations

---

## Contributing

See the main README.md for contribution guidelines.

## License

LP is released under the MIT License.
