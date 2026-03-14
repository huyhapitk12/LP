# LP Language

LP is a statically typed language with a Python-like surface syntax and a native C compilation pipeline. The compiler parses `.lp` source, generates C, and then uses a host toolchain to build a native executable.

This repository now uses a documentation layout with three goals:

- teach LP from zero
- document the real compiler and runtime behavior that exists today
- call out partial, unsupported, and platform-specific behavior clearly

## Highlights

- Python-like syntax for `def`, `class`, `if`, `for`, `while`, `import`, and exceptions.
- Native modules for `math`, `random`, `time`, `os`, `sys`, `http`, `json`, `sqlite`, `thread`, `memory`, and `platform`.
- Compile-time checked `private` and `protected` members.
- Built-in REPL, test runner, profiler, watch mode, build, package, and C API export commands.
- Cross-target entrypoints for `windows-x64`, `linux-x64`, `linux-arm64`, and `macos-arm64`.

## Requirements

- Windows: MSYS2 UCRT64 GCC is the supported local build path.
- Linux: a system C compiler such as GCC.
- macOS: Xcode Command Line Tools or another GCC-compatible toolchain.

## Build The Compiler

```bash
# Windows
build.bat

# POSIX shell path
./compile.sh

# Generic make path
make
```

## Quick Start

Create `hello.lp`:

```lp
def main():
    print("Hello from LP")

main()
```

Run it with the repo build output:

```bash
# Windows
build\lp.exe hello.lp

# Linux or macOS
./build/lp hello.lp
```

Generate C only:

```bash
lp hello.lp -o hello.c
```

Compile a native executable:

```bash
lp hello.lp -c hello.exe
```

Run regression tests:

```bash
lp test examples
```

## CLI Snapshot

```bash
lp file.lp
lp file.lp -o out.c
lp file.lp -c out.exe
lp file.lp -asm out.s
lp test examples
lp profile file.lp
lp watch file.lp
lp build file.lp --release --strip
lp package file.lp --format zip
lp export file.lp -o api_name
lp export file.lp --library -o api_name
```

## Cross-Target Snapshot

```bash
lp build file.lp --target windows-x64
lp build file.lp --target linux-x64
lp build file.lp --target linux-arm64
lp build file.lp --target macos-arm64
```

If the requested cross-toolchain is missing, LP fails early with a clear toolchain error.

## Documentation

Start here:

- **[FEATURES.md](docs/FEATURES.md)** - Comprehensive feature status (supported, partial, unsupported)
- [Documentation Map](docs/00_Documentation_Map.md)
- [Installation and Setup](docs/01_Installation_and_Setup.md)
- [First Programs](docs/02_First_Programs.md)
- [Language Basics](docs/03_Language_Basics.md)
- [Runtime Modules](docs/08_Runtime_Modules.md)
- [CLI and Tooling](docs/09_CLI_and_Tooling.md)
- [Language Reference](docs/14_Language_Reference.md)
- [Huong dan tieng Viet](docs/guide_vi.md)

## Accuracy Notes

The docs describe the compiler and runtime as they exist in this repository today.

- `http.get(...)` is supported. `http.post(...)` is not.
- `json.loads(...)` and `json.dumps(...)` are supported. `json.parse(...)` is not.
- `thread.spawn(...)` currently accepts only named LP functions, with at most one argument, and the worker must return `int` or `void`.
- `parallel for` emits OpenMP-style code, but actual parallel execution depends on an OpenMP-capable toolchain configuration.
- `lp export --library` currently emits a `.dll` library name even outside Windows-oriented workflows.

## License

This project is released under the [MIT License](LICENSE).
