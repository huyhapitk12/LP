# Multithreading Architecture
LP eliminates the Global Interpreter Lock (GIL) by mapping parallel `spawn` routines directly onto native OS threads.

## Creating Threads
You can start a named worker function asynchronously and join it for an `int` result.

```python
import thread

def compute(limit: int) -> int:
    total = 0
    for i in range(limit):
        total = total + i
    return total

def main():
    t1: thread = thread.spawn(compute, 100)
    res1: int = thread.join(t1)
    print(res1)
```

`thread.spawn(...)` currently accepts named LP functions with 0 or 1 argument.
If the worker returns `void`, `thread.join(...)` returns `0`.

## Atomic Locks
Prevent memory access conflicts structurally by acquiring `LpLock`:

```python
import thread

lock: lock = thread.lock_init()

def mutate():
    thread.lock_acquire(lock)
    # Perform modification securely!
    thread.lock_release(lock)
```