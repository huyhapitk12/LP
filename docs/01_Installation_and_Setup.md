# Installation and Setup

## What you'll learn

This guide shows how to build the LP compiler on Windows, Linux, and macOS, how to verify that the toolchain is usable, and how to diagnose the most common setup failures.

## Prerequisites

- A checkout of this repository.
- A working C toolchain for your host platform.

## Concepts

LP is a compiler front-end plus runtime headers.

- The compiler executable is built from `compiler/src/*.c`.
- **Default: LP compiles directly to x86-64 assembly** (no GCC/LLVM required!)
- **Optional: LP can generate C code** and use a host C compiler for maximum compatibility.
- Runtime headers are loaded from the repository `runtime/` directory at compile time.

### Compilation Backends

| Backend | Command | Dependencies | Use Case |
|---------|---------|--------------|----------|
| **Native ASM** (default) | `lp file.lp` | binutils (~5MB) | Fast, lightweight |
| **GCC C** | `lp file.lp --gcc` | GCC (~500MB) | Maximum optimization |
| **C output** | `lp file.lp -o out.c` | Any C compiler | Portability |

In the rest of the docs, `lp` means one of these:

- an installed `lp` binary on your `PATH`
- `build\lp.exe` on Windows
- `./build/lp` on Linux or macOS
- `./build/lp-posix.exe` inside MSYS2 or Cygwin bash

## Windows Setup

### Recommended local toolchain

Use MSYS2 UCRT64 GCC.

Expected compiler path:

```text
C:\msys64\ucrt64\bin\gcc.exe
```

### Build the compiler

```bat
build.bat
```

Expected result:

```text
build\lp.exe
```

### Verify the compiler

```bat
build\lp.exe --help
build\lp.exe test examples
```

## Linux Setup

Install a standard toolchain first.

Example for Ubuntu or Debian:

```bash
sudo apt-get update
sudo apt-get install -y build-essential
```

Build the compiler with either path:

```bash
make
# or
./compile.sh
```

Verify it:

```bash
./build/lp --help
./build/lp test examples
```

## macOS Setup

Install Xcode Command Line Tools:

```bash
xcode-select --install
```

Then build the compiler:

```bash
make
# or
./compile.sh
```

Verify it:

```bash
./build/lp --help
```

## Build Entry Points

### `build.bat`

- Host platform: Windows.
- Expected toolchain: MSYS2 UCRT64 GCC.
- Output: `build\lp.exe`.

### `compile.sh`

- Host platform: POSIX shell environments.
- Output on Linux/macOS: `build/lp`.
- Output in Cygwin or MSYS shell: `build/lp-posix.exe`.

### `make`

- Host platform: Linux, macOS, or compatible environments.
- Output: `build/lp` on POSIX and `build/lp.exe` when `OS=Windows_NT` is set.

## Verify The Full Local Pipeline

Create `hello.lp`:

```lp
def main():
    print("hello")

main()
```

Run these checks:

```bash
lp hello.lp
lp hello.lp -o hello.c
lp hello.lp -c hello.exe
lp test examples
```

If all four succeed, your local LP host build path is healthy.

## Cross-Target Verification

You can also verify that `--target` failure modes are wired correctly even before installing cross-compilers.

```bash
lp build hello.lp --target linux-x64
```

Expected behavior today:

- LP should fail quickly if the required cross-toolchain is not installed.
- The error should name the missing target compiler instead of failing silently later.

## Examples

### Windows quick session

```bat
build.bat
build\lp.exe test examples
build\lp.exe examples\test_type_inference.lp
```

### Linux quick session

```bash
make
./build/lp test examples
./build/lp examples/test_type_inference.lp
```

## Common errors and limitations

### `Error: GCC not found`

Meaning:

- LP could not find `gcc` or `cc`.
- On Windows, the expected MSYS2 path is missing.

Fix:

- Install MSYS2 UCRT64 GCC on Windows.
- Install `build-essential` on Linux.
- Install Xcode Command Line Tools on macOS.

### `Error: cannot find lp_runtime.h`

Meaning:

- LP found the compiler executable but could not locate the `runtime/` headers.

Fix:

- Run the compiler from the repository build output or from a properly installed layout.
- Keep `runtime/` next to the compiler installation as expected by the current runtime search logic.

### `make` is missing on Windows

Meaning:

- The Windows machine does not have a POSIX build environment.

Fix:

- Use `build.bat` instead.
- Or install `make` inside MSYS2 if you want to verify the POSIX path locally.

### `Target toolchain ... was not found`

Meaning:

- `lp build --target ...` correctly detected that the requested cross-compiler is not available.

Fix:

- Install the target-specific toolchain such as `x86_64-linux-gnu-gcc` or `aarch64-apple-darwin-gcc`.

### `Permission denied` while rebuilding `build\lp.exe`

Meaning:

- The compiler executable is still running or held open by another process.

Fix:

- Close the running process and rebuild again.

## See also

- [Documentation Map](00_Documentation_Map.md)
- [First Programs](02_First_Programs.md)
- [Build, Distribution, and Cross-Targets](10_Build_Distribution_and_Cross_Targets.md)
- [Troubleshooting](13_Troubleshooting.md)
