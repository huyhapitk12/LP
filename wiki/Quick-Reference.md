# Quick Reference

A quick syntax reference for LP language.

## 🔤 Variables & Types

```lp
# Type inference
x = 42
name = "LP"
pi = 3.14
flag = True

# Explicit types
count: int = 100
ratio: float = 0.5
label: str = "text"
active: bool = False
```

## 📝 Functions

```lp
# Basic function
def add(a: int, b: int) -> int:
    return a + b

# Default arguments
def greet(name: str = "World") -> str:
    return "Hello " + name

# Variadic function
def sum_all(*args) -> int:
    total = 0
    for i in args:
        total += i
    return total
```

## 📦 Classes

```lp
class Animal:
    def speak(self) -> str:
        return "sound"

class Dog(Animal):
    def speak(self) -> str:
        return "woof"

# Access modifiers
class Example:
    public_var: int = 1
    private _internal: int = 2
    protected _protected: int = 3
```

## 🔄 Control Flow

```lp
# Conditionals
if x > 10:
    print("big")
elif x > 5:
    print("medium")
else:
    print("small")

# For loops
for i in range(5):
    print(i)

for i in range(0, 10, 2):
    print(i)

# While loops
while condition:
    break
    continue

# Pattern matching
value = 2
match value:
    case 1:
        print("one")
    case 2:
        print("two")
    case _:
        print("other")
```

## ⚡ Parallel Computing

```lp
# Basic parallel for
parallel for i in range(1000000):
    process(i)

# With @settings
@settings(threads=8, schedule="dynamic", chunk=100)
def parallel_process(data: list) -> list:
    results = []
    parallel for item in data:
        results.append(process(item))
    return results

# GPU acceleration
@settings(device="gpu", gpu_id=0)
def gpu_compute(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i * i
    return result
```

## 🔒 Security

```lp
# Rate limiting
@security(rate_limit=100)
def api_endpoint():
    pass

# Authentication required
@security(auth=True, level=3)
def protected_function():
    pass

# Admin only
@security(admin=True)
def admin_function():
    pass

# Read-only mode
@security(readonly=True)
def read_function():
    pass
```

## 📚 Collections

```lp
# Lists
nums = [1, 2, 3]
nums.append(4)
nums[0]  # access

# Dicts
obj = {"name": "LP", "version": "0.1.1"}
obj["name"]

# Tuples
pair = (1, 2)
first = pair[0]
second = pair[1]

# Sets — experimental, may not be stable
unique = {1, 2, 3, 2, 1}  # ⚠️ experimental
```

## 🧵 Strings

```lp
# F-strings
name = "LP"
version = "0.1.0"
greeting = f"Hello from {name} v{version}!"

# Methods
text.upper()
text.lower()
text.strip()
text.split(",")
text.find("sub")
text.replace("old", "new")
```

## 🎯 Lambda

```lp
# Single-line
add = lambda x, y: x + y

# Multi-line
process = lambda x:
    result: int = x * 2
    return result
```

## 📂 File I/O

```lp
# With statement
with open("file.txt", "r") as f:
    content = f.read()

with open("out.txt", "w") as f:
    f.write("content")
```

## ⚠️ Error Handling

```lp
try:
    risky_operation()
except:
    print("error occurred")
finally:
    cleanup()

raise "custom error"
```

## 📦 Imports

```lp
import math
import time
import http
import json
import sqlite

# Using modules
math.sqrt(16)
time.time()
http.get("https://api.example.com")
json.loads('{"key": "value"}')
```

## 🔧 CLI Commands

```bash
lp file.lp                    # Run file (native ASM backend)
lp file.lp --gcc              # Preferred verification path on Windows
lp file.lp -o out.c           # Generate C
lp file.lp -c output          # Compile to exe
lp file.lp -asm out.s         # Generate LP native assembly
lp test examples              # Run tests
lp profile file.lp            # Profile
```
