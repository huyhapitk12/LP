# LP Language Feature Status

> Last updated: 2025-01-18
> Version: 0.1.0-beta

This document provides a comprehensive overview of LP language features, their implementation status, and any known limitations.

## Quick Reference

| Category | Fully Supported | Partial | Not Yet |
|----------|----------------|---------|---------|
| Core Syntax | 15+ | 2 | 4 |
| Runtime Modules | 10 | 2 | 0 |
| Operators | 20+ | 0 | 0 |
| CLI Tools | 10 | 0 | 1 |

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
# Parallel for loop (requires OpenMP)
parallel for i in range(1000000):
    process_item(i)
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

### ❌ Not Supported Yet

#### HTTP POST

```lp
# ✅ Works
response = http.get("https://api.example.com/data")

# ❌ Not implemented
response = http.post("https://api.example.com/submit", data)
```

**Workaround:** Use HTTP GET with query parameters or use a C library.

#### JSON Parse

```lp
# ✅ Works
data = json.loads('{"name": "LP"}')
text = json.dumps(data)

# ❌ Not implemented
data = json.parse('{"name": "LP"}')  # Use loads instead
```

#### Dictionary Comprehensions

```lp
# ❌ Not supported
squares = {x: x*x for x in range(10)}

# ✅ Workaround
squares = {}
for x in range(10):
    squares[x] = x * x
```

#### Generators / Yield

```lp
# ❌ Not supported (keyword reserved)
def gen():
    yield 1
    yield 2
```

#### Decorators

```lp
# ❌ Not supported
@decorator
def func():
    pass
```

#### Async/Await

```lp
# ❌ Not supported
async def fetch():
    result = await http.get(url)
    return result
```

#### Pattern Matching

```lp
# ❌ Not supported
match value:
    case 1:
        print("one")
    case _:
        print("other")
```

#### Type Unions

```lp
# ❌ Not supported
def process(x: int | str) -> int:
    pass
```

#### Generic Types

```lp
# ❌ Not supported
class Box[T]:
    value: T
```

#### F-Strings

```lp
# ❌ Not supported
name = "LP"
print(f"Hello {name}")  # Syntax error

# ✅ Workaround
name = "LP"
print("Hello " + name)
print("Hello " + str(name))
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
# Run directly
lp file.lp

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

1. **HTTP POST** - Full HTTP method support
2. **Dictionary comprehensions** - Similar to list comprehensions
3. **Generators** - `yield` keyword implementation
4. **Better error messages** - More descriptive compiler errors
5. **Package manager** - Dependency management
6. **Standard library expansion** - More modules and utilities

---

## Contributing

See the main README.md for contribution guidelines.

## License

LP is released under the MIT License.
