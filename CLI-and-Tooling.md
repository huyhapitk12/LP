# CLI and Tooling

## What you'll learn

This guide documents every public `lp` command path exposed by the current compiler entry point, including direct file execution, testing, profiling, watch mode, build, packaging, and C API export.

## Prerequisites

- Read [Installation and Setup](Installation-and-Setup) first.

## Command Summary

| Command | Purpose |
| --- | --- |
| `lp` | Start the interactive REPL. |
| `lp file.lp` | Parse, compile, and run an LP file directly. |
| `lp file.lp -o out.c` | Generate C only. |
| `lp file.lp -c out.exe` | Compile a named native executable. |
| `lp file.lp -asm out.s` | Generate assembly. |
| `lp test [dir]` | Run LP tests in a directory. |
| `lp profile file.lp` | Build and run the profiling path. |
| `lp watch file.lp` | Recompile and rerun on file changes. |
| `lp build file.lp ...` | Build a standalone artifact with build flags. |
| `lp package file.lp --format ...` | Build then archive the result. |
| `lp export file.lp ...` | Generate a C API header and optionally a shared library. |

## `lp`

Starts the REPL.

Useful REPL commands:

- `.help`
- `.clear`
- `.show`
- `.exit`
- `.quit`

## `lp file.lp`

Runs an LP file directly.

```bash
lp hello.lp
```

Behavior:

- parse source
- generate temporary C
- compile temporary native executable
- run it
- clean up the temporary executable

## `lp file.lp -o out.c`

Generate C only.

```bash
lp hello.lp -o hello.c
```

Use this when you want to inspect the generated C or compile it manually.

## `lp file.lp -c out.exe`

Compile a reusable native executable.

```bash
lp hello.lp -c hello.exe
```

On POSIX hosts you would usually omit the `.exe` suffix yourself.

## `lp file.lp -asm out.s`

Generate assembly.

```bash
lp hello.lp -asm hello.s
```

## `lp test [dir]`

Run LP regression tests.

```bash
lp test examples
```

Behavior today:

- scans for `test_*.lp`
- parses each file
- finds `def test_*()` functions
- compiles per-test harness executables
- runs them and prints pass or fail status

## `lp profile file.lp`

Profile a program.

```bash
lp profile hello.lp
```

Current behavior:

- LP wraps the generated `main` body with timing instrumentation
- the profiler prints a total execution report
- this is a simple built-in profiling path, not a full production profiler UI

## `lp watch file.lp`

Watch a file and rerun on changes.

```bash
lp watch hello.lp
```

Behavior:

- monitors the input file modification time
- recompiles and reruns when the file changes
- clears the screen between runs on supported hosts
- exits with `Ctrl+C`

## `lp build file.lp`

Build a standalone native executable.

```bash
lp build app.lp
```

Supported flags:

- `--release`
- `--static`
- `--strip`
- `--no-console`
- `-o output_name`
- `--target windows-x64|linux-x64|linux-arm64|macos-arm64`

Example:

```bash
lp build app.lp --release --strip -o app.exe
```

## `lp package file.lp`

Build and archive an application.

```bash
lp package app.lp --format zip
lp package app.lp --format tar.gz
```

Current behavior:

- calls `lp build file.lp --release --strip`
- archives the resulting host artifact
- accepts `zip` and `tar.gz`

## `lp export file.lp`

Generate a C API header.

```bash
lp export api.lp -o my_api
```

Generate header plus shared library:

```bash
lp export api.lp --library -o my_api
```

Current behavior:

- without `--library`: writes a header only
- with `--library`: writes a header and builds a `.dll` shared library artifact

See [C API Export](C-API-Export) for details.

## `lp --help`

Shows the built-in usage summary.

## Common errors and limitations

- Direct run, `-c`, `-asm`, `test`, `profile`, `watch`, `build`, `package`, and library export all depend on a working C compiler.
- `lp build --target ...` fails early if the target toolchain is missing.
- `lp package` only knows `zip` and `tar.gz`.
- `lp export --library` currently writes `.dll` naming in the implementation even though the header has non-Windows visibility guards.
- Watch mode recompiles the program repeatedly, so it shares most of the same toolchain failure modes as direct execution.

## See also

- [Build and Distribution](Build-and-Distribution)
- [C API Export](C-API-Export)
- [Troubleshooting](Troubleshooting)
- [Language Reference](Language-Reference)
