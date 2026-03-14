# 🚀 LP Language

<p align="center">
  <img src="docs/images/lp-logo.png" alt="LP Logo" width="180">
</p>

<p align="center">
  <strong>A lightweight programming language with Python-like syntax that compiles to native machine code</strong>
</p>

<p align="center">
  <strong>⚡ No GCC/LLVM required! ⚡</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-0.3.0-blue.svg" alt="Version">
  <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License">
  <img src="https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg" alt="Platform">
  <img src="https://img.shields.io/badge/language-C-555555.svg" alt="Language">
</p>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#documentation">Documentation</a> •
  <a href="#comparison">Comparison</a> •
  <a href="CONTRIBUTING.md">Contributing</a>
</p>

---

## ✨ Why LP?

| Feature | LP | Python | C | Go | Rust |
|---------|----|----|---|-----|------|
| **Compilation** | Native ASM | Interpreted | Native | Native | Native |
| **Dependencies** | ~5MB (binutils) | ~100MB+ | ~500MB (GCC) | ~200MB | ~1GB+ |
| **Startup Time** | ⚡ Instant | 🐢 Slow | ⚡ Instant | ⚡ Instant | ⚡ Instant |
| **Performance** | 🚀 Native | 🐌 Slow | 🚀 Native | 🚀 Native | 🚀 Native |
| **Syntax** | 🐍 Python-like | 🐍 Python | 😰 Verbose | 🎯 Clean | 😰 Complex |
| **Learning Curve** | ✅ Easy | ✅ Easy | ❌ Hard | ⚠️ Medium | ❌ Hard |

## 🎯 Highlights

- 🔥 **Native Compilation** - Direct LP → Assembly → Machine Code (no GCC/LLVM needed!)
- 🪶 **Ultra-lightweight** - Only requires binutils (`as` + `ld`, ~5MB)
- 🐍 **Python-like Syntax** - Familiar `def`, `class`, `if`, `for`, `while`, `import`
- ⚡ **Modern Features** - F-Strings, Pattern Matching, Decorators, Async/Await, Generics, Type Unions
- 🧵 **Built-in Modules** - `math`, `random`, `time`, `os`, `sys`, `http`, `json`, `sqlite`, `thread`, `memory`
- 🚀 **Native OpenMP** - Automatic parallel execution with `parallel for`
- 🎯 **Optimizations** - Constant folding, dead code elimination, loop unrolling

---

## 📋 Requirements

- **Linux x86-64**: binutils (`as`, `ld`) - usually pre-installed
- **Windows**: MSYS2 UCRT64 or MinGW
- **macOS**: Xcode Command Line Tools

---

## 🚀 Quick Start

### Build The Compiler

```bash
# POSIX shell
./compile.sh

# Or using make
make
```

### Hello World

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

---

## 📖 Features

### 🎨 Modern Syntax

```lp
# Variables with type inference
name = "LP"
version = 0.3
active = True

# Explicit types
count: int = 100
ratio: float = 0.5
label: str = "text"

# F-Strings
greeting = f"Hello from {name} v{version}!"
print(greeting)  # Output: Hello from LP v0.3!
```

### 🧩 Pattern Matching

```lp
match value:
    case 1:
        print("one")
    case 2:
        print("two")
    case n if n > 10:
        print("big")
    case _:
        print("other")
```

### 🎭 Decorators

```lp
@settings(threads=4)
def parallel_task():
    print("Running with 4 threads")
```

### ⚡ Async/Await

```lp
async def fetch_data(url: str) -> str:
    return "data from " + url

async def main():
    data = await fetch_data("https://example.com")
    print(data)
```

### 📦 Generic Types

```lp
class Box[T]:
    value: T
    
    def get(self) -> T:
        return self.value

# Usage
int_box: Box[int] = Box()
int_box.set(42)
```

### 🚀 Parallel Computing

```lp
# OpenMP-style parallel for (auto-enabled!)
parallel for i in range(1000000):
    process_item(i)
```

### 📚 Built-in Modules

```lp
import math
import time
import http
import json
import sqlite
import thread
import memory

# Math
pi: float = math.pi
result = math.sqrt(16)  # 4.0

# HTTP
response: str = http.get("https://api.example.com")

# JSON
data = json.loads('{"name": "LP"}')
text = json.dumps(data)

# SQLite
db = sqlite.connect("mydb.db")
results = sqlite.query(db, "SELECT * FROM users")

# Threading
t = thread.spawn(worker_function, 42)
result = thread.join(t)

# Memory Management
arena = memory.arena_new(1024)
ptr = memory.arena_alloc(arena, 64)
```

---

## 🛠️ CLI Commands

```bash
lp file.lp                    # Run with native backend (default)
lp file.lp --gcc              # Run with GCC backend (optional)
lp file.lp -o out.c           # Generate C code
lp file.lp -c out.exe         # Compile to executable
lp file.lp -asm out.s         # Generate assembly
lp test examples              # Run tests
lp profile file.lp            # Profile execution
lp watch file.lp              # Hot reload mode
lp build file.lp --release    # Build optimized executable
lp package file.lp            # Package for distribution
```

---

## 📂 Project Structure

```
LP/
├── compiler/          # Compiler source code (C)
├── runtime/           # Runtime library
├── examples/          # Example programs
├── docs/              # Documentation
├── vscode-lp/         # VSCode extension
├── skills/            # AI assistant skills
├── Makefile           # Build configuration
├── compile.sh         # Build script (POSIX)
└── build.bat          # Build script (Windows)
```

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| [FEATURES.md](docs/FEATURES.md) | Comprehensive feature status |
| [Installation and Setup](docs/01_Installation_and_Setup.md) | Getting started |
| [First Programs](docs/02_First_Programs.md) | Tutorial |
| [Language Basics](docs/03_Language_Basics.md) | Core concepts |
| [Runtime Modules](docs/08_Runtime_Modules.md) | Built-in modules |
| [CLI and Tooling](docs/09_CLI_and_Tooling.md) | Command-line tools |
| [Language Reference](docs/14_Language_Reference.md) | Complete reference |
| [Parallel Computing](docs/15_Parallel_Computing.md) | OpenMP integration |
| [Huong dan tieng Viet](docs/guide_vi.md) | Vietnamese guide |

---

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Ways to Contribute

- 🐛 Report bugs
- 💡 Suggest features
- 📝 Improve documentation
- 🔧 Submit pull requests
- ⭐ Star the repository!

---

## 📊 Comparison

| Feature | LP | Python | C | Go |
|---------|----|----|---|-----|
| Compilation | Native ASM | Interpreted | Native | Native |
| Dependencies | ~5MB (binutils) | ~100MB+ | ~500MB (GCC) | ~200MB |
| Startup Time | Instant | Slow | Instant | Instant |
| Performance | Native | Slow | Native | Native |
| Memory Safe | Partial | Yes | No | Yes |

---

## 📜 License

MIT License - See [LICENSE](LICENSE)

---

<p align="center">
  Made with ❤️ by <a href="https://github.com/huyhapitk12">Ho Quang Huy</a>
</p>
