# Runtime Modules

## What you'll learn

This guide documents the public runtime surface that the current LP compiler knows how to lower to native code, including verified modules, builtins that map directly into the runtime, and the most important limitations.

## Prerequisites

- Read [Language Basics](03_Language_Basics.md) first.
- Read [Expressions and Collections](04_Expressions_and_Collections.md) if you want more background on strings and collection access.

## Module Status Overview

| Module or surface | Status | Notes |
| --- | --- | --- |
| `math` | Supported | Direct mapping to native math helpers. |
| `random` | Supported | Seed, random, randint, uniform. |
| `time` | Supported | `time()` and `sleep()`. |
| `os` and `os.path` | Supported | Native file-system helpers. |
| `sys` | Supported | Platform string, argv helpers, exit, size helpers. |
| `http` | Partially supported | `http.get(...)` only. |
| `json` | Partially supported | `json.loads(...)` and `json.dumps(...)` only. |
| `sqlite` | Supported | `connect`, `execute`, `query`. |
| `thread` | Partially supported | Public surface is available, but `spawn` has strict compiler rules. |
| `memory` | Supported | Arena, pool, and `cast`. |
| `platform` | Supported | `os`, `arch`, `cores`. |
| `open()` and file handles | Supported | File-oriented runtime helpers. |
| `numpy` import path | Internal or experimental | Present in source, not documented as stable public workflow yet. |
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

- `http.get(url)`

### Unsupported API

- `http.post(...)`
- any other HTTP verb not explicitly documented here

### Example

```lp
import http

def main():
    body: str = http.get("https://jsonplaceholder.typicode.com/posts/1")
    print(body)

main()
```

### Limitation

The compiler intentionally rejects unsupported `http.*` calls with a compile-time error.

## `json`

### Supported API

- `json.loads(text)`
- `json.dumps(value)`

### Unsupported API

- `json.parse(...)`
- undocumented helpers not listed here

### Example

```lp
import json

def main():
    data = json.loads('{"name":"LP","ok":true}')
    print(str(data["name"]))
    print(json.dumps(data))

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

The worker restrictions described in [Concurrency and Parallelism](07_Concurrency_and_Parallelism.md) are part of the supported public contract.

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

- Unknown `http.*` and `json.*` calls are intentionally rejected when they are outside the currently supported API subset.
- `thread.spawn(...)` is not a generic callback runner. It requires a named LP worker function.
- `memory.cast(...)` expects a class type in the documented public workflow.
- `os.path.*` is a stable native path for file-system helpers. Arbitrary nested module calls outside documented cases are not guaranteed.
- Source paths for `numpy` and arbitrary Python imports exist in the compiler, but they are not documented as stable public workflows yet because the build path is still environment-sensitive.

## See also

- [Concurrency and Parallelism](07_Concurrency_and_Parallelism.md)
- [CLI and Tooling](09_CLI_and_Tooling.md)
- [Language Reference](14_Language_Reference.md)
