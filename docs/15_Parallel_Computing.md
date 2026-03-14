# Parallel and GPU Computing in LP

LP provides built-in support for parallel computing using OpenMP and GPU acceleration.

## Overview

LP integrates OpenMP directly into the language through the `parallel` keyword, allowing you to easily parallelize loops for multi-core CPU execution. OpenMP is enabled by default when compiling.

## Compilation Backends

LP supports two compilation backends:

1. **Native ASM Backend (Default)**: Direct LP → Assembly → Machine Code
2. **GCC Backend (Optional)**: LP → C → GCC → Machine Code

```bash
lp file.lp              # Native compilation (default)
lp file.lp --gcc        # GCC backend with OpenMP support
```

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

## Parallel Settings with @settings Decorator

LP provides fine-grained control over parallel execution through the `@settings` decorator:

### Basic Usage

```lp
# Configure thread count
@settings(threads=8)
def parallel_process(data: list) -> list:
    results = []
    parallel for i in range(len(data)):
        results.append(process(data[i]))
    return results

# Set scheduling policy and chunk size
@settings(threads=4, schedule="dynamic", chunk=100)
def process_large_dataset(data: list) -> int:
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

# Auto device selection (GPU if available, otherwise CPU)
@settings(device="auto")
def auto_parallel(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i
    return result

# Unified memory for GPU (seamless CPU/GPU data sharing)
@settings(device="gpu", unified=True)
def unified_memory_compute(size: int) -> list:
    results = []
    parallel for i in range(size):
        results.append(i)
    return results
```

### Available Settings

| Setting | Type | Description | Default |
|---------|------|-------------|---------|
| `threads` | int | Number of threads (0 = auto) | 0 (auto) |
| `schedule` | string | Scheduling policy: "static", "dynamic", "guided", "auto" | "static" |
| `chunk` | int | Chunk size for scheduling | 0 (auto) |
| `device` | string | Execution device: "cpu", "gpu", "auto" | "cpu" |
| `gpu_id` | int | GPU device ID | 0 |
| `unified` | bool | Use unified memory for GPU | false |

### Scheduling Policies

- **static**: Divide work evenly across threads (default, best for uniform workloads)
- **dynamic**: Work in chunks assigned on-demand (best for uneven workloads)
- **guided**: Decreasing chunk sizes (hybrid approach)
- **auto**: Let OpenMP runtime decide

### Generated C Code

When you use `@settings`, LP generates appropriate OpenMP pragmas:

```lp
@settings(threads=4, schedule="dynamic", chunk=100)
def process(data: list) -> int:
    result = 0
    parallel for item in data:
        result += item
    return result
```

Generates:

```c
void lp_process(LpArray data) {
    /* === PARALLEL/GPU SETTINGS === */
    lp_parallel_set_threads(4);
    lp_parallel_configure(4, "dynamic", 100, 0, 0);
    /* === END PARALLEL/GPU SETTINGS === */
    
    int64_t result = 0;
    #pragma omp parallel for num_threads(4) schedule(dynamic, 100)
    for (int64_t i = 0; i < data.len; i++) {
        result += data.items[i];
    }
}
```

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

LP includes GPU abstraction for CUDA/OpenCL support:

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

### GPU Execution with @settings

```lp
# Use specific GPU device
@settings(device="gpu", gpu_id=0)
def gpu_compute(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i * i
    return result

# Auto-select best device (GPU if available)
@settings(device="auto")
def auto_device_compute(data: list) -> list:
    results = []
    parallel for item in data:
        results.append(process(item))
    return results

# Unified memory for seamless CPU/GPU data sharing
@settings(device="gpu", unified=True)
def unified_compute(data: list) -> list:
    results = []
    parallel for i in range(len(data)):
        results.append(data[i] * 2)
    return results
```

### GPU Runtime Functions

The `lp_gpu.h` module provides GPU management:

```lp
import gpu

# Get device count
num_gpus: int = gpu.device_count()

# Get device info
device = gpu.get_device(0)
print("Name:", device.name)
print("Memory:", device.total_memory)
print("Compute Units:", device.compute_units)

# Select device
gpu.select_device(0)

# Synchronize (wait for GPU operations to complete)
gpu.sync()
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

- ✅ **@settings Decorator**: Fine-grained parallel/GPU control (implemented)
- ✅ **GPU Device Selection**: Select GPU device with `device="gpu"` (implemented)
- ✅ **Scheduling Policies**: Static, dynamic, guided, auto (implemented)
- 🔧 **GPU Kernel Generation**: Automatic CUDA/OpenCL code generation (in progress)
- 🔧 **Distributed Computing**: Multi-node parallel execution (planned)
- 🔧 **Automatic Parallelization**: Compiler auto-detects parallelizable loops (planned)
- 🔧 **Parallel Algorithms**: Built-in parallel sort, reduce, scan, etc. (planned)
