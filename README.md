# LP Language

LP is a lightweight, statically typed language with Python-like syntax that compiles directly to native machine code. **No C compiler required!**

## Highlights

- **Native Compilation**: Direct LP → Assembly → Machine Code (no GCC/LLVM needed!)
- **Ultra-lightweight**: Only requires binutils (`as` + `ld`, ~5MB)
- **Python-like Syntax**: Familiar `def`, `class`, `if`, `for`, `while`, `import`
- **Built-in Modules**: `math`, `random`, `time`, `os`, `sys`, `http`, `json`, `sqlite`, `thread`, `memory`
- **Native OpenMP**: Automatic parallel execution with `parallel for`
- **Optimizations**: Constant folding, dead code elimination, loop unrolling

## Requirements

- **Linux x86-64**: binutils (`as`, `ld`) - usually pre-installed
- **Windows**: MSYS2 UCRT64 or MinGW
- **macOS**: Xcode Command Line Tools

## Build The Compiler

```bash
# POSIX shell
./compile.sh

# Or using make
make
```

## Quick Start

Create `hello.lp`:

```lp
def main():
    print("Hello from LP!")

main()
```

Run it with native compilation:

```bash
# Native compilation (default - no GCC needed!)
./build/lp hello.lp

# Or with GCC backend (optional)
./build/lp hello.lp --gcc
```

## Compilation Modes

```bash
lp file.lp              # Native ASM → Machine Code (default, no GCC!)
lp file.lp --gcc        # Use GCC backend
lp file.lp -o out.c     # Generate C code
lp file.lp -c out.exe   # Compile to executable
lp file.lp -asm out.s   # Generate assembly
```

## Features

### Parallel Computing

```lp
# OpenMP-style parallel for (auto-enabled!)
parallel for i in range(1000000):
    process_item(i)
```

### Optimizations

The compiler performs several optimizations:

- **Constant Folding**: `1 + 2 * 3` → `7` at compile time
- **Dead Code Elimination**: Removes unreachable code
- **Loop Unrolling**: Unrolls small loops for speed

### Built-in Modules

```lp
import math
import time
import os
import sys
import http
import json
import sqlite
import thread
import memory

# Use them
pi: float = math.pi
time.sleep(1.0)
response: str = http.get("https://api.example.com")
```

## CLI Commands

```bash
lp file.lp                    # Run with native backend
lp file.lp --gcc              # Run with GCC backend
lp file.lp -o out.c           # Generate C code
lp file.lp -c out.exe         # Compile to executable
lp test examples              # Run tests
lp profile file.lp            # Profile execution
lp watch file.lp              # Hot reload mode
lp build file.lp --release    # Build optimized executable
lp package file.lp            # Package for distribution
```

## Documentation

- **[FEATURES.md](docs/FEATURES.md)** - Comprehensive feature status
- [Installation and Setup](docs/01_Installation_and_Setup.md)
- [First Programs](docs/02_First_Programs.md)
- [Language Basics](docs/03_Language_Basics.md)
- [Runtime Modules](docs/08_Runtime_Modules.md)
- [CLI and Tooling](docs/09_CLI_and_Tooling.md)
- [Language Reference](docs/14_Language_Reference.md)
- [Parallel Computing](docs/15_Parallel_Computing.md)
- [Huong dan tieng Viet](docs/guide_vi.md)

## Comparison

| Feature | LP | Python | C | Go |
|---------|----|----|---|-----|
| Compilation | Native ASM | Interpreted | Native | Native |
| Dependencies | ~5MB (binutils) | ~100MB+ | ~500MB (GCC) | ~200MB |
| Startup Time | Instant | Slow | Instant | Instant |
| Performance | Native | Slow | Native | Native |
| Memory Safe | Partial | Yes | No | Yes |

## License

MIT License - See [LICENSE](LICENSE)
