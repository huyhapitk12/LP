# LP Language

**Python Syntax • C Performance**

LP is a high-performance programming language that combines Python's readable syntax with C's execution speed. LP code is transpiled to optimized C, then compiled with GCC `-O3`, delivering **300–700x faster** execution than Python.

## ⚡ Quick Start

```python
# hello.lp
print("Hello, LP!")

def fibonacci(n: int) -> int:
    if n <= 1:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)

print(fibonacci(40))
```

```bash
# Run directly
lp hello.lp

# Generate C code
lp hello.lp -o hello.c

# Compile to executable
lp hello.lp -c hello.exe
```

## 🚀 Performance

| Benchmark | LP | Python | Speedup |
|---|---|---|---|
| Fibonacci(40) | 0.000s | 0.005s | **∞** |
| Sum of Squares (10M) | 0.010s | 3.34s | **334x** |
| Loop (100M) | 0.005s | 3.58s | **715x** |

## 📦 Build from Source

### Requirements
- GCC (via MSYS2/MinGW on Windows, or system GCC on Linux/macOS)

### Windows
```batch
build.bat
```

### Linux / macOS
```bash
make
```

### Using the Compiler
```bash
# Add the build directory to PATH, then:
lp your_file.lp          # Run directly
lp your_file.lp -o out.c # Transpile to C
lp your_file.lp -c out   # Compile to native binary
```

## 📁 Project Structure

```
LP/
├── compiler/src/    # Compiler source (C)
│   ├── main.c       # Entry point & CLI
│   ├── lexer.c/h    # Tokenizer
│   ├── parser.c/h   # Parser → AST
│   ├── ast.c/h      # AST definitions
│   └── codegen.c/h  # C code generator
├── runtime/         # Runtime headers (included in generated C)
│   ├── lp_runtime.h
│   ├── lp_dict.h
│   ├── lp_array.h
│   ├── lp_set.h
│   ├── lp_tuple.h
│   └── ...
├── examples/        # Example LP programs
├── docs/            # Language documentation
├── build.bat        # Windows build script
├── compile.sh       # Linux build script
├── Makefile         # Cross-platform build
├── lp.bat           # Windows runner
└── lp.ps1           # PowerShell runner
```

## ✨ Features

- **Python-compatible syntax** — `def`, `class`, `for`, `if`, `import`, etc.
- **Static typing** — Type annotations for maximum performance
- **Data structures** — Lists, dicts, sets, tuples, list comprehensions
- **OOP** — Classes with methods and attributes
- **Lambda & generators** — Functional programming support
- **Variadic functions** — `*args` and `**kwargs`
- **Exception handling** — `try/except/finally/raise`
- **File I/O** — `with open() as f:` context managers
- **Standard library** — `math`, `time`, `random`, `os`, `sys`
- **NumPy bridge** — Native array operations via `import numpy`

## 📚 Documentation

- [LP Language Guide (English)](docs/TEXTBOOK.md)
- [Hướng dẫn học LP (Tiếng Việt)](docs/guide_vi.md)

## 📄 License

[MIT License](LICENSE)
