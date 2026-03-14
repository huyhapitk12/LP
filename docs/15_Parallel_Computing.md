# Parallel and GPU Computing in LP

LP provides built-in support for parallel computing using OpenMP and GPU acceleration.

## Overview

LP integrates OpenMP directly into the language through the `parallel` keyword, allowing you to easily parallelize loops for multi-core CPU execution. The compiler automatically adds `-fopenmp` flag when compiling, so you don't need to worry about compiler flags.

## Quick Start

### Basic Parallel For Loop

```lp
# Simple parallel for loop - processes iterations in parallel
parallel for i in range(1000000):
    process_item(i)
```

This generates C code with OpenMP:
```c
#pragma omp parallel for
for (int64_t lp_i = 0; lp_i < 1000000; lp_i++) {
    process_item(lp_i);
}
```

### Parallel For with Range

```lp
# Parallel for with start, end, step
parallel for i in range(0, 100, 5):
    print(i)
```

### Example: Parallel Data Processing

```lp
import math

def analyze_sample(sample_id: int) -> float:
    baseline: float = math.sin(sample_id * 0.01)
    smoothing: float = math.cos(sample_id * 0.015)
    return baseline * smoothing

def main():
    print("Processing one million independent samples...")
    
    parallel for sample_id in range(1000000):
        analyze_sample(sample_id)
    
    print("Finished sample processing.")

main()
```

## Compilation

OpenMP is automatically enabled when you use `parallel for` in your code. Just compile normally:

```bash
# Compile and run directly
lp your_program.lp

# Compile to executable
lp your_program.lp -c your_program

# Generate C code only
lp your_program.lp -o your_program.c
```

The LP compiler automatically adds `-fopenmp` when it detects parallel loops.

## Parallel Settings (Future Feature)

LP will support fine-grained control over parallel execution through the `@settings` decorator:

```lp
# Set number of threads
@settings(threads=8)
parallel for i in range(1000):
    process(i)

# Set scheduling policy
@settings(schedule="dynamic", chunk=100)
parallel for i in range(10000):
    process(i)

# GPU execution
@settings(device="gpu")
parallel for i in range(1000000):
    compute(i)
```

### Available Settings

| Setting | Type | Description |
|---------|------|-------------|
| `threads` | int | Number of threads (0 = auto) |
| `schedule` | string | Scheduling policy: "static", "dynamic", "guided", "auto" |
| `chunk` | int | Chunk size for scheduling |
| `device` | string | Execution device: "cpu", "gpu", "auto" |
| `gpu_id` | int | GPU device ID |
| `unified` | bool | Use unified memory for GPU |

## Runtime Functions

The `lp_parallel.h` module provides runtime functions for parallel computing:

### Thread Management

```lp
import parallel

# Get number of available CPU cores
cores: int = parallel.cores()

# Get/set thread count
max_threads: int = parallel.max_threads()
parallel.set_threads(8)

# Get current thread ID (in parallel region)
tid: int = parallel.thread_id()
```

### Timing

```lp
import parallel

start: float = parallel.wtime()

# ... parallel computation ...

end: float = parallel.wtime()
print("Elapsed time:", end - start)
```

### Parallel Reductions

```lp
import parallel

# Sum reduction (calculated in parallel)
data: list = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
total: int = parallel.sum(data)

# Max/Min
maximum: int = parallel.max(data)
minimum: int = parallel.min(data)
```

## GPU Computing

LP includes GPU abstraction for future CUDA/OpenCL support:

### GPU Detection

```lp
import gpu

# Check GPU availability
if gpu.is_available():
    print("GPU detected:", gpu.device_name(0))
    print("Memory:", gpu.total_memory(0))
else:
    print("No GPU available, using CPU")
```

### GPU Execution (Planned)

```lp
@settings(device="gpu", threads=256)
parallel for i in range(1000000):
    result[i] = compute(data[i])
```

## Best Practices

### 1. Choose Appropriate Loop Granularity

```lp
# Good: Large iteration count benefits from parallelization
parallel for i in range(1000000):
    result[i] = expensive_computation(i)

# Avoid: Small iteration counts may have overhead
parallel for i in range(10):  # Sequential is faster
    result[i] = simple_operation(i)
```

### 2. Avoid Data Dependencies

```lp
# Good: Independent iterations
parallel for i in range(1000):
    result[i] = data[i] * 2

# Avoid: Dependencies between iterations
parallel for i in range(1, 1000):
    result[i] = result[i-1] + data[i]  # Race condition!
```

### 3. Use Reductions for Accumulators

```lp
# Sequential version
total: int = 0
for i in range(1000):
    total += data[i]

# Parallel version requires explicit reduction handling
# (Currently need to implement manually in LP)
```

## Performance Tips

1. **Use OpenMP scheduling**: For uneven workloads, use `schedule="dynamic"` 
2. **Set appropriate chunk sizes**: Balance overhead vs load balancing
3. **Avoid false sharing**: Ensure threads don't write to nearby memory locations
4. **Use thread-local storage**: For thread-specific data

## Compilation Flags

When compiling generated C code with OpenMP:

```bash
# Basic OpenMP
gcc -fopenmp program.c -o program

# With optimizations
gcc -O3 -fopenmp -march=native program.c -o program -lm

# Set thread count at runtime
OMP_NUM_THREADS=8 ./program
```

## Architecture

```
┌─────────────────────────────────────────┐
│             LP Source Code              │
│   parallel for i in range(N):          │
│       process(i)                       │
└────────────────┬────────────────────────┘
                 │ LP Compiler
                 ▼
┌─────────────────────────────────────────┐
│            Generated C Code             │
│   #pragma omp parallel for             │
│   for (int64_t i = 0; i < N; i++)      │
│       process(i);                      │
└────────────────┬────────────────────────┘
                 │ C Compiler (gcc -fopenmp)
                 ▼
┌─────────────────────────────────────────┐
│           Native Binary                │
│   - Multi-threaded execution           │
│   - Automatic work distribution        │
│   - Thread pool management             │
└─────────────────────────────────────────┘
```

## Future Roadmap

- **GPU Kernel Generation**: Automatic CUDA/OpenCL code generation
- **Distributed Computing**: Multi-node parallel execution
- **Automatic Parallelization**: Compiler auto-detects parallelizable loops
- **Parallel Algorithms**: Built-in parallel sort, reduce, scan, etc.
