# LP Language Wiki

This wiki documents the LP language, compiler, runtime modules, and competitive programming workflow.

## Start Here

If you are new to LP, read these pages in order:

1. [Installation and Setup](Installation-and-Setup)
2. [First Programs](First-Programs)
3. [Language Basics](Language-Basics)
4. [Quick Reference](Quick-Reference)
5. [Troubleshooting](Troubleshooting)

## Choose A Path

### Learn the language

- [Language Basics](Language-Basics) for syntax and everyday usage
- [Language Reference](Language-Reference) for the complete public surface
- [Expressions and Collections](Expressions-and-Collections) for strings, lists, dicts, tuples, and collection behavior
- [Object-Oriented Programming](Object-Oriented-Programming) for classes and inheritance
- [Error Handling](Error-Handling) for `try`, `except`, `finally`, and `raise`

### Understand current feature support

- [Feature Overview](Features) for a high-level overview
- [Feature Status](Feature-Status) for implementation status and caveats
- [Runtime Modules](Runtime-Modules) for built-in modules and their public contracts

### Build, run, and package programs

- [CLI and Tooling](CLI-and-Tooling) for `lp` commands
- [Build and Distribution](Build-and-Distribution) for artifact builds and targets
- [C API Export](C-API-Export) for header and library export
- [LP Linter](LP-Linter) for static checking

### Work with concurrency, parallelism, and security

- [Concurrency and Parallelism](Concurrency-and-Parallelism) for `thread.spawn`, `thread.join`, locks, and `parallel for`
- [Parallel and GPU Computing](Parallel-Computing) for OpenMP/GPU-oriented workflows
- [Security Overview](Security-Explained) for the `@security` model
- [Security Reference](Security-Module-Reference) for detailed security options
- [Security Calling Patterns](Calling-Security-Functions) for examples and calling patterns

### Use LP for competitive programming

- [DSA Module Guide](DSA-Module-Guide) for fast I/O and data structures
- [Performance Guide](Performance-Guide) for current performance guidance
- [Compilation Troubleshooting](Compilation-Errors) for common CP-specific build failures
- [Examples Cookbook](Examples-Cookbook) for worked examples

## Current Documentation Policy

This wiki now prioritizes verified behavior over planned behavior.

- The native ASM backend is the compiler default.
- On Windows, `--gcc` is the recommended verification path.
- Literal `match` cases, `_` wildcard, and capture-pattern guards (`case n if n > 10`) are fully stable.
- Set literals and advanced threading paths are documented conservatively.

## Common Destinations

- [Quick Reference](Quick-Reference) for a fast syntax cheat sheet
- [Runtime Modules](Runtime-Modules) for module APIs such as `time`, `json`, `sqlite`, and `thread`
- [Feature Status](Feature-Status) when you need to know whether a feature is fully supported, partial, or experimental
- [Troubleshooting](Troubleshooting) when a command or build path fails

## Project Links

- [GitHub repository](https://github.com/huyhapitk12/LP)
- [Issue tracker](https://github.com/huyhapitk12/LP/issues)
- [Releases](https://github.com/huyhapitk12/LP/releases)
