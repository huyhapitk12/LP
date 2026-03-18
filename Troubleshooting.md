# Troubleshooting

## What you'll learn

This guide maps common LP failures to the most likely causes and the fastest fixes.

## Prerequisites

None.

## Setup and build failures

### Symptom: `Error: GCC not found`

Cause:

- no usable C compiler is on `PATH`
- on Windows, the expected MSYS2 GCC path is missing

Fix:

- Windows: install MSYS2 UCRT64 GCC and rebuild with `build.bat`
- Linux: install `build-essential`
- macOS: install Xcode Command Line Tools

### Symptom: `Error: cannot find lp_runtime.h`

Cause:

- the compiler executable cannot locate the `runtime/` include directory

Fix:

- run the compiler from the repository layout or install runtime headers beside it in the expected structure

### Symptom: `make` is not available on the current machine

Cause:

- the host does not have a POSIX build environment

Fix:

- use `build.bat` on Windows
- or install `make` inside MSYS2, WSL, Linux, or macOS

### Symptom: `Permission denied` while rebuilding `build\lp.exe`

Cause:

- the executable is still running or locked by another process

Fix:

- close the running process and rebuild

## Parser and syntax failures

### Symptom: `expected ':'`

Cause:

- a colon is missing after `def`, `class`, `if`, `elif`, `else`, `for`, `while`, `try`, `except`, `finally`, or `with`

Fix:

- inspect the failing line and add the colon

### Symptom: `expected indented block`

Cause:

- the block body is missing or not indented

Fix:

- indent the block body
- or use `pass` for an empty block

### Symptom: the compiler prints nearby lines and a highlight bar

Cause:

- this is LP's enhanced parser error display working as intended

Fix:

- focus on the highlighted line first
- then read the hint below the context output

## Access control failures

### Symptom: access to `private` or `protected` member is rejected

Cause:

- the current scope is not allowed to read, write, or call that member

Fix:

- move the access inside the declaring class or a valid subclass for `protected`
- change the member to public if the access should be part of the public API

## Module API failures

### Symptom: `http.post` is rejected

Cause:

- only `http.get(...)` is supported today

Fix:

- use `http.get(...)`
- or add the missing API to the runtime and compiler before documenting or relying on it

### Symptom: `json.parse` is rejected

Cause:

- only `json.loads(...)` and `json.dumps(...)` are supported today

Fix:

- switch to `json.loads(...)` or `json.dumps(...)`

## Threading failures

### Symptom: `thread.spawn` rejects the worker

Cause:

One of these conditions is true:

- the worker is not a named LP function
- the worker has more than one parameter
- the worker returns something other than `int` or `void`
- the caller passed the wrong number of arguments to the worker

Fix:

- wrap the logic in a named LP function
- reduce the worker signature to zero or one parameter
- change the return type to `int` or `void`

## Tooling failures

### Symptom: `lp test examples` reports compile errors for test harnesses

Cause:

- one or more `test_*.lp` files do not parse or compile

Fix:

- run the failing file directly with `lp file.lp -o out.c` if you need more codegen visibility

### Symptom: `lp watch file.lp` keeps failing

Cause:

- the watched file still has a parse, codegen, or toolchain error

Fix:

- fix the underlying compiler or syntax error first
- then save the file again to trigger a clean run

### Symptom: `lp profile file.lp` fails during compilation

Cause:

- the profile path still depends on the same host compiler and runtime include path as other build modes

Fix:

- fix GCC discovery or runtime include discovery first

### Symptom: `Unknown package format`

Cause:

- `lp package` only accepts `zip` or `tar.gz`

Fix:

- use one of the supported formats

### Symptom: `lp export --library` fails

Cause:

- host compiler is missing
- runtime include path is missing
- shared-library build failed

Fix:

- verify the normal host build path first with `lp file.lp -c out.exe`
- then retry the export path

## Cross-target failures

### Symptom: `Target toolchain ... was not found`

Cause:

- the requested cross-compiler is not installed on the machine

Fix:

Install the compiler that matches your target:

- `windows-x64` -> `x86_64-w64-mingw32-gcc`
- `linux-x64` -> `x86_64-linux-gnu-gcc`
- `linux-arm64` -> `aarch64-linux-gnu-gcc`
- `macos-arm64` -> `aarch64-apple-darwin-gcc`

## Common errors and limitations

- Unsupported APIs should be documented as unsupported, not treated as mysterious compiler bugs.
- `parallel for` syntax may compile even when your toolchain is not configured for real OpenMP parallel execution.
- Some advanced source paths exist in the compiler but are not yet stable public workflows. If the docs do not present them as supported, treat them carefully.

## See also

- [Installation and Setup](Installation-and-Setup)
- [CLI and Tooling](CLI-and-Tooling)
- [Build and Distribution](Build-and-Distribution)
