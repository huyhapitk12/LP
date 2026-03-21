# Build and Distribution

## What you'll learn

This guide explains the difference between host builds and cross-target builds, how `lp build` chooses artifact names, which toolchains are expected, and what the current packaging and distribution limitations are.

## Prerequisites

- Read [Installation and Setup](Installation-and-Setup).
- Read [CLI and Tooling](CLI-and-Tooling) for the command overview.

## Concepts

### Host build

A host build produces a native executable for the machine doing the compilation.

Examples:

```bash
lp build app.lp
lp build app.lp --release --strip
```

### Cross-target build

A cross-target build asks LP to use a different compiler for a different OS or architecture.

Example:

```bash
lp build app.lp --target linux-x64
```

This requires the matching cross-toolchain to be installed already.

## Supported build flags

- `--release`
- `--static`
- `--strip`
- `--no-console`
- `-o output_name`
- `--target windows-x64|linux-x64|linux-arm64|macos-arm64`

## Artifact Naming Rules

### Host builds

- Windows host builds default to `<basename>.exe`.
- POSIX host builds default to `<basename>`.

### Cross-target builds

- If `--target windows-x64` is used and no `-o` is provided, the default artifact name keeps the host executable suffix behavior.
- If `--target` is used with a non-Windows target and no `-o` is provided, LP defaults to `<basename>` without `.exe`.

This behavior was intentionally cleaned up so non-Windows targets do not default to Windows-style names.

## Target Toolchains

Current target-to-compiler mapping in the implementation:

| Target | Expected compiler |
| --- | --- |
| `windows-x64` | `x86_64-w64-mingw32-gcc` |
| `linux-x64` | `x86_64-linux-gnu-gcc` |
| `linux-arm64` | `aarch64-linux-gnu-gcc` |
| `macos-arm64` | `aarch64-apple-darwin-gcc` |

## Release Builds

Example:

```bash
lp build app.lp --release --strip
```

Current effect:

- higher optimization path
- link-time optimization
- extra aliasing and loop flags
- strip symbols when requested

## Static Builds

Example:

```bash
lp build app.lp --static
```

This forwards `-static` to the host toolchain. Whether it succeeds depends on your toolchain and libraries.

## GUI-style Windows builds

Example:

```bash
lp build app.lp --no-console
```

Current effect:

- forwards `-mwindows`
- mainly meaningful for Windows-oriented GUI subsystem builds

## Packaging

`lp package` currently wraps a host build and then archives it.

Supported archive formats:

- `zip`
- `tar.gz`

Example:

```bash
lp package app.lp --format zip
```

Implementation detail:

- `zip` packaging uses the host `tar` command in zip-archive mode
- Windows expects `tar.exe`
- POSIX expects `tar`

## Cross-Target Verification Strategy

### Minimal verification

Even without the cross-toolchain installed, you can verify that LP reports the missing tool cleanly:

```bash
lp build app.lp --target linux-x64
```

### Real verification

To prove the full path, you need:

1. the target compiler installed
2. the LP compiler built on the host machine
3. a successful `lp build ... --target ...` run
4. ideally, a real test run on the target OS or CI runner

## Current repository status

This repository includes host build entrypoints for:

- `build.bat`
- `compile.sh`
- `Makefile`

It also includes CI coverage intended to exercise Windows, Linux, and macOS host build paths.

## Common errors and limitations

- Missing target toolchain is the most common cross-target failure. LP now reports that directly.
- Cross-target success still depends on the requested compiler actually existing on your machine or CI runner.
- `parallel for` code generation is separate from cross-target logic. A successful target build does not automatically mean OpenMP runtime acceleration is enabled.
- `lp export --library` still uses `.dll` naming in the current implementation, so it is not yet a generic cross-platform shared-library packaging story.
- `lp package` packages host builds. It is not a full cross-platform release manager.

## See also

- [CLI and Tooling](CLI-and-Tooling)
- [C API Export](C-API-Export)
- [Troubleshooting](Troubleshooting)
