# Language Reference

## What you'll learn

This file is a compact lookup reference for LP syntax, builtins, module surface, and command summary.

## Prerequisites

None.

## Core syntax

### Comments

```lp
# comment
```

### Variable declaration and inference

```lp
x: int = 42
name = "LP"
```

### Function definition

```lp
def add(a: int, b: int) -> int:
    return a + b
```

Functions with default values:

```lp
def greet(name: str = "World") -> str:
    return "Hello " + name
```

Variadic functions:

```lp
def sum_all(*args) -> int:
    total = 0
    for i in args:
        total = total + i
    return total
```

### Class definition

```lp
class Player(Entity):
    health: int = 100
```

### Control flow

```lp
if cond:
    pass
elif other:
    pass
else:
    pass

for i in range(5):
    print(i)

while cond:
    break
```

### Exceptions

```lp
try:
    risky()
except:
    print("handled")
finally:
    print("cleanup")

raise "error"
```

### File-oriented `with`

```lp
with open("file.txt", "r") as f:
    print(f.read())
```

### Lambda

```lp
add = lambda x, y: x + y
```

Multiline form:

```lp
mul = lambda x, y:
    result: int = x * y
    return result
```

### Concurrency

```lp
parallel for i in range(1000):
    work(i)
```

## Access modifiers

- public: default
- `private`
- `protected`

## Builtins

Common builtins documented for normal use:

- `print(x)`
- `len(x)`
- `range(stop)`
- `range(start, stop)`
- `range(start, stop, step)`
- `open(path, mode)`
- `str(x)`
- `int(x)`
- `float(x)`
- `bool(x)`

## Literal forms

- integers: `1`
- floats: `3.14`
- booleans: `true`, `false`, `True`, `False`
- string: `"text"`, `'text'`, `"""multiline"""`
- null-like value: `None`, `none`
- list: `[1, 2, 3]`
- dict: `{"k": 1}`
- set: `{1, 2, 3}`
- tuple: `(1, 2)`

## Operators

### Arithmetic
| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `a + b` |
| `-` | Subtraction | `a - b` |
| `*` | Multiplication | `a * b` |
| `/` | Division (float) | `a / b` |
| `//` | Floor division | `a // b` |
| `%` | Modulo | `a % b` |
| `**` | Power | `a ** b` |

### Bitwise
| Operator | Description | Example |
|----------|-------------|---------|
| `&` | AND | `a & b` |
| `\|` | OR | `a \| b` |
| `^` | XOR | `a ^ b` |
| `~` | NOT | `~a` |
| `<<` | Left shift | `a << n` |
| `>>` | Right shift | `a >> n` |

### Comparison
| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal | `a == b` |
| `!=` | Not equal | `a != b` |
| `<` | Less than | `a < b` |
| `>` | Greater than | `a > b` |
| `<=` | Less or equal | `a <= b` |
| `>=` | Greater or equal | `a >= b` |

### Logical
| Operator | Description | Example |
|----------|-------------|---------|
| `and` | Logical AND | `a and b` |
| `or` | Logical OR | `a or b` |
| `not` | Logical NOT | `not a` |
| `is` | Identity | `a is None` |
| `in` | Membership | `x in list` |

### Assignment
| Operator | Example | Equivalent |
|----------|---------|------------|
| `=` | `a = b` | - |
| `+=` | `a += b` | `a = a + b` |
| `-=` | `a -= b` | `a = a - b` |
| `*=` | `a *= b` | `a = a * b` |
| `/=` | `a /= b` | `a = a / b` |
| `&=` | `a &= b` | `a = a & b` |
| `\|=` | `a \|= b` | `a = a \| b` |
| `^=` | `a ^= b` | `a = a ^ b` |
| `<<=` | `a <<= n` | `a = a << n` |
| `>>=` | `a >>= n` | `a = a >> n` |

## String methods

Supported method-style calls:

- `upper()` - Convert to uppercase
- `lower()` - Convert to lowercase
- `strip()` - Remove leading/trailing whitespace
- `lstrip()` - Remove leading whitespace
- `rstrip()` - Remove trailing whitespace
- `find(sub)` - Find substring, returns index or -1
- `replace(old, new)` - Replace all occurrences
- `count(sub)` - Count substring occurrences
- `split(delim)` - Split into array
- `join(parts)` - Join array with separator
- `startswith(prefix)` - Check prefix
- `endswith(suffix)` - Check suffix
- `isdigit()` - Check if all digits
- `isalpha()` - Check if all letters
- `isalnum()` - Check if alphanumeric

## Collection notes

- nested subscript such as `rows[0]["n"]` is supported and verified
- list comprehensions exist but are currently documented as partial and numeric-focused
- dictionary comprehensions are not documented as supported public syntax today

## Type annotations

### Basic types

```lp
x: int = 42
y: float = 3.14
name: str = "LP"
flag: bool = True
```

### Special types

```lp
data: val = json.loads(s)      # Dynamic JSON value
ptr: ptr = memory.cast(addr)   # Raw pointer
arena: arena = memory.arena_new(1024)  # Memory arena
pool: pool = memory.pool_new(64, 10)   # Memory pool
```

## Runtime modules

### `math`

Representative API:

- `pi`, `e`, `tau` - Constants
- `sin(x)`, `cos(x)`, `tan(x)` - Trigonometry
- `sqrt(x)`, `pow(x, y)` - Power functions
- `factorial(n)`, `gcd(a, b)`, `lcm(a, b)` - Integer math
- `isnan(x)`, `isinf(x)` - Float checks
- `floor(x)`, `ceil(x)`, `round(x)` - Rounding
- `abs(x)`, `log(x)`, `exp(x)` - Other functions

### `random`

- `seed(n)` - Set seed
- `random()` - Float in [0, 1)
- `randint(a, b)` - Integer in [a, b]
- `uniform(a, b)` - Float in [a, b]

### `time`

- `time()` - Current timestamp (seconds)
- `sleep(seconds)` - Sleep for duration

### `os`

- `getcwd()` - Current working directory
- `remove(path)` - Delete file
- `rename(src, dst)` - Rename/move file
- `mkdir(path)` - Create directory
- `sep` - Path separator (`/` or `\`)
- `name` - OS name (`"posix"` or `"nt"`)

### `os.path`

- `join(a, b)` - Join path components
- `exists(path)` - Check if path exists
- `isfile(path)` - Check if file
- `isdir(path)` - Check if directory
- `basename(path)` - Get filename
- `dirname(path)` - Get directory
- `getsize(path)` - Get file size

### `sys`

- `platform` - Platform string (`"linux"`, `"win32"`, `"darwin"`)
- `maxsize` - Maximum integer value
- `exit(code)` - Exit program
- `getrecursionlimit()` - Get recursion limit
- `argv_len()` - Argument count
- `argv_get(i)` - Get argument by index

### `http`

- `get(url)` - HTTP GET request (returns response body)
- `post(url, data)` - HTTP POST request
- `put(url, data)` - HTTP PUT request
- `delete(url)` - HTTP DELETE request
- `patch(url, data)` - HTTP PATCH request

### `json`

- `loads(s)` - Parse JSON string to val
- `dumps(v)` - Serialize val to JSON string
- `parse(s)` - Alias for `loads`

### `sqlite`

- `connect(path)` - Open database
- `execute(db, sql)` - Execute SQL
- `query(db, sql)` - Query and return results

### `thread`

- `spawn(worker, arg)` - Spawn thread with worker function
- `join(thread)` - Wait for thread completion
- `lock_init()` - Create mutex lock
- `lock_acquire(lock)` - Acquire lock
- `lock_release(lock)` - Release lock

**Restrictions:**
- Worker must be a named LP function
- Worker can have 0 or 1 argument only
- Worker must return `int` or `void`

### `memory`

- `arena_new(size)` - Create arena allocator
- `arena_alloc(arena, size)` - Allocate from arena
- `arena_reset(arena)` - Reset arena
- `arena_free(arena)` - Free arena
- `pool_new(chunk_size, count)` - Create pool allocator
- `pool_alloc(pool)` - Allocate from pool
- `pool_free(pool, ptr)` - Free to pool
- `pool_destroy(pool)` - Destroy pool
- `cast(ptr)` - Cast pointer to object

### `platform`

- `os()` - OS name string
- `arch()` - Architecture string
- `cores()` - Number of CPU cores

### `numpy` (Comprehensive)

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
- `np.reshape(arr, rows, cols)` / `np.reshape2d(arr, rows, cols)`
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

## Feature Status

### Fully Supported ✅

| Feature | Status | Notes |
|---------|--------|-------|
| Variables & type inference | ✅ | |
| Functions | ✅ | Including default args, *args |
| Classes | ✅ | With inheritance |
| `private`/`protected` | ✅ | Compile-time checked |
| `if`/`elif`/`else` | ✅ | |
| `for` loops | ✅ | With `range()` |
| `while` loops | ✅ | With `break`, `continue` |
| `try`/`except`/`finally` | ✅ | |
| `raise` | ✅ | |
| `with` statement | ✅ | |
| Lambda (single-line) | ✅ | |
| Lambda (multiline) | ✅ | |
| String methods | ✅ | Full set |
| List literals | ✅ | |
| Dict literals | ✅ | |
| Set literals | ✅ | |
| Tuple literals | ✅ | |
| Bitwise operators | ✅ | All 6 operators |
| Compound assignment | ✅ | `+=`, `-=`, etc. |
| `parallel for` | ✅ | Requires OpenMP toolchain |
| `@settings` decorator | ✅ | Parallel/GPU configuration |
| `@security` decorator | ✅ | Security features |
| F-strings | ✅ | `f"value: {x}"` |
| **Dictionary comprehensions** | ✅ | `{k: v for x in iter}` |
| **`yield` / Generators** | ✅ | Basic generator pattern |
| **`http.post`** | ✅ | HTTP POST requests |
| **`json.parse`** | ✅ | Alias for `json.loads` |

### Partially Supported ⚠️

| Feature | Status | Notes |
|---------|--------|-------|
| List comprehensions | ⚠️ | Numeric-focused, limited |
| `thread.spawn` | ⚠️ | Strict worker restrictions |
| `numpy` module | ⚠️ | Basic functions only |
| Typed exceptions | ⚠️ | Generic runtime behavior |

### Not Supported Yet ❌

| Feature | Status | Notes |
|---------|--------|-------|
| (All core features are now at least partially supported) | | |

### Partially Implemented (Use with Caution) ⚠️

| Feature | Status | Notes |
|---------|--------|-------|
| Async/await | ⚠️ | Syntactic sugar only - synchronous execution |
| Type unions | ⚠️ | Parsed but no runtime type checking |
| Generic types | ⚠️ | Parsed but no type specialization |

### Supported Decorators ✅

| Decorator | Description | Example |
|-----------|-------------|---------|
| `@settings` | Configure parallel/GPU execution | `@settings(threads=8, device="gpu")` |
| `@security` | Security features (rate limit, auth) | `@security(level=3, auth=True)` |

#### @settings Options

```lp
@settings(threads=4)                    # Set thread count
@settings(schedule="dynamic", chunk=100) # Scheduling policy
@settings(device="gpu", gpu_id=0)       # GPU execution
@settings(device="auto")                # Auto-select best device
@settings(unified=True)                 # Unified GPU memory
```

#### @security Options

```lp
@security(level=3)                      # Security level (0-4)
@security(auth=True)                    # Require authentication
@security(rate_limit=100)               # Requests per minute
@security(admin=True)                   # Require admin access
@security(readonly=True)                # Read-only mode
@security(validate=True, sanitize=True) # Input/output handling
```

## CLI summary

```text
lp
lp file.lp
lp file.lp -o out.c
lp file.lp -c out.exe
lp file.lp -asm out.s
lp test [dir]
lp profile file.lp
lp watch file.lp
lp build file.lp [flags]
lp package file.lp --format zip|tar.gz
lp export file.lp [-o name] [--library]
```

## Common errors and limitations

- If a feature is not listed here as supported, assume it needs confirmation before treating it as stable public surface.
- Cross-target builds need matching external toolchains.
- C API export currently has Windows-oriented shared-library naming in the implementation.
- Integer division `//` follows Python semantics (floor division).
- Modulo `%` follows Python semantics (result has same sign as divisor).

## See also

- [Documentation Map]
- [Runtime Modules](Runtime-Modules)
- [CLI and Tooling](CLI-and-Tooling)
- [Troubleshooting](Troubleshooting)
