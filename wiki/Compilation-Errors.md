# Compilation Troubleshooting

This page is the compile-focused companion to [Troubleshooting](Troubleshooting).

The detailed error workflow has been merged into the main troubleshooting guide so there is one canonical place for build failures, runtime failures, and debugging steps.

## Use This Page For Fast Triage

### 1. Verify the backend path

On Windows, start with:

```bash
lp file.lp --gcc
lp build file.lp --release --gcc
```

If you are checking the native default path, remember that `lp file.lp` uses the ASM backend and expects a working assembler/linker toolchain.

### 2. Inspect generated C

```bash
lp file.lp -o output.c
```

Then inspect the generated file around the failing line.

### 3. Recompile the generated C manually

```bash
gcc -Wall -Wextra -std=c11 output.c -o output
```

This is often the fastest way to surface missing symbols, invalid lowering, or host-toolchain warnings.

### 4. Use the canonical guide

For the full troubleshooting flow, go to [Troubleshooting](Troubleshooting), especially:

- compile and backend checks
- generated-C debugging
- runtime debugging
- common LP coding mistakes

## Common Compile-Failure Categories

| Category | Typical symptom |
| --- | --- |
| LP syntax error | parser or codegen error before C compilation |
| lowering/codegen issue | generated C fails under GCC |
| missing toolchain | `gcc`, `as`, or `ld` not found |
| stale assumptions from docs | example parses, but current build path is not reliable end-to-end |

## Related Pages

- [Troubleshooting](Troubleshooting)
- [CLI and Tooling](CLI-and-Tooling)
- [Installation and Setup](Installation-and-Setup)
