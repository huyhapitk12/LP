# Concurrency and Parallelism

## What you'll learn

This guide covers `thread.spawn(...)`, `thread.join(...)`, locks, `parallel for`, and the documented public contract you should rely on today.

## Prerequisites

- Read [Language Basics](Language-Basics) first.
- Read [Runtime Modules](Runtime-Modules) if you want module-by-module API detail.

## Syntax And Concepts

### `thread.spawn(...)`

Current supported contract:

- the worker must be a named LP function
- the worker should take zero arguments in the current verified build
- the worker must return `int` or `void`

Treat this as the verified public usage contract for the current release.

### `thread.join(...)`

`thread.join(...)` waits for the worker and returns an `int` result.

### Locks

The public thread module also exposes:

- `thread.lock_init()`
- `thread.lock_acquire(lock)`
- `thread.lock_release(lock)`

These are useful when multiple threads touch shared in-memory state.

### `parallel for`

LP supports a `parallel for` syntax for numeric loop forms:

```lp
parallel for i in range(1000):
    work(i)
```

Current implementation detail:

- LP emits an OpenMP-style `#pragma omp parallel for` in generated C.
- Actual parallel execution depends on the host toolchain and flags.
- LP compiler automatically adds `-fopenmp` when parallel loops are detected.

### `@settings` Decorator

LP provides the `@settings` decorator for fine-grained control over parallel execution:

```lp
# Configure thread count
@settings(threads=8)
def parallel_process(data: list) -> list:
    results = []
    parallel for item in data:
        results.append(process(item))
    return results

# Set scheduling policy and chunk size
@settings(threads=4, schedule="dynamic", chunk=100)
def process_uneven_workload(data: list) -> int:
    count = 0
    parallel for item in data:
        count += item
    return count

# GPU execution
@settings(device="gpu", gpu_id=0)
def gpu_compute(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i * i
    return result

# Auto-select best device
@settings(device="auto")
def auto_parallel(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i
    return result
```

Available settings:

| Setting | Type | Description | Default |
|---------|------|-------------|---------|
| `threads` | int | Number of threads (0 = auto) | 0 |
| `schedule` | string | "static", "dynamic", "guided", "auto" | "static" |
| `chunk` | int | Chunk size for scheduling | 0 |
| `device` | string | "cpu", "gpu", "auto" | "cpu" |
| `gpu_id` | int | GPU device ID | 0 |
| `unified` | bool | Use unified memory for GPU | false |

## Examples

### Thread worker and join

```lp
import thread

def worker() -> int:
    return 42

t: thread = thread.spawn(worker)
result: int = thread.join(t)
print(result)
```

### Thread plus SQLite example

The regression suite verifies a real threaded workflow in `tests/regression/test_thread_sqlite.lp`.

Key pattern:

```lp
import sqlite
import thread

def store_worker(worker_id: int) -> int:
    db: sqlite_db = sqlite.connect("thread_test.db")
    sqlite.execute(db, "PRAGMA busy_timeout = 5000;")
    return worker_id + 10
```

### `parallel for`

```lp
import math

def compute(idx: int) -> float:
    return math.sin(idx * 0.1) * math.cos(idx * 0.2)

parallel for i in range(1000):
    value = compute(i)
```

## Anti-Patterns

These should be treated as unsupported usage in the current release:

- passing a lambda directly to `thread.spawn(...)`
- using a worker with one or more arguments in the current build
- using a worker that returns a type other than `int` or `void`
- passing callback arguments to `thread.spawn(...)` in the current build
- relying on capture-pattern guards or other non-thread-safe dynamic callback shapes

## Common errors and limitations

- `thread.spawn(...)` is intentionally strict. If you want to thread work, wrap it in a named LP function first.
- `thread.join(...)` returns `int`, so model worker result channels accordingly.
- `parallel for` does not currently provide automatic reduction semantics for shared accumulators.
- The current docs treat `parallel for` as partially supported because runtime speedup depends on your toolchain configuration, not just LP syntax.

## See also

- [Runtime Modules](Runtime-Modules)
- [Build and Distribution](Build-and-Distribution)
- [Examples Cookbook](Examples-Cookbook)
- [Language Reference](Language-Reference)
