# Troubleshooting

This is the canonical troubleshooting guide for LP build failures, runtime failures, and debugging workflow.

## Start With The Right Backend

- `lp file.lp` uses the native ASM backend by default.
- On Windows, start diagnosis with `lp file.lp --gcc` or `lp build file.lp --release --gcc`.
- If the default path fails early with assembler or linker errors, verify the `as`/`ld` toolchain separately from GCC.

## Compile And Build Failures

### LP parse or codegen error before C compilation

Try:

```bash
lp file.lp -v
lp file.lp -o output.c
```

Use `-v` to surface the compiler stage, then inspect generated C when lowering succeeds far enough.

### Generated C fails under GCC

Recompile the generated C manually:

```bash
lp file.lp -o output.c
gcc -Wall -Wextra -std=c11 output.c -o output
```

This is the fastest way to spot:

- invalid lowering
- missing runtime symbols
- host-toolchain warnings promoted into errors

### Toolchain setup problems

Typical symptoms:

- `gcc not found`
- `as` or `ld` not found
- `undefined reference`
- `unknown type name 'size_t'`

Checks:

- verify the compiler/toolchain path from [Installation and Setup](Installation-and-Setup)
- verify the runtime headers are reachable
- verify the build script includes all required source files
- use C11 or GNU11, not older C modes for `_Generic` support

## Common LP Coding Mistakes

| Issue | Better approach |
| --- | --- |
| Missing output in CP code | Call `dsa.flush()` before exit |
| 2D list-style indexing in flat arrays | Use `idx = i * row_size + j` |
| Treating `ptr` as invalid | `ptr` is valid, but it is a low-level type |
| Assuming every parsed feature is production-ready | Check [Feature Status](Feature-Status) for partial/experimental features |

## Runtime Failures

### No output

Usually caused by missing `dsa.flush()` in competitive-programming code.

### Crash or invalid memory access

Common causes:

- array out-of-bounds access
- misuse of low-level memory helpers
- deep recursion
- bad assumptions about partially supported features

### Wrong input or wrong answer

Check:

- sample input shape
- off-by-one indexing
- flat-array row size
- whether the example relies on a feature that is only partially supported

## Practical Debug Workflow

1. Run `lp file.lp -v`.
2. If needed, generate `output.c` with `lp file.lp -o output.c`.
3. Recompile the C output manually with GCC warnings enabled.
4. Reduce the LP source to the smallest failing snippet.
5. Compare the feature you are using against [Feature Status](Feature-Status) and [Runtime Modules](Runtime-Modules).

## Competitive Programming Checklist

- Use `dsa.flush()` for buffered output.
- Pre-allocate hot arrays when possible.
- Use flat indexing for 2D numeric arrays.
- Prefer the verified GCC path when checking a new snippet on Windows.
- Re-test with tiny inputs before scaling up.

## Related Pages

- [Compilation Troubleshooting](Compilation-Errors)
- [Installation and Setup](Installation-and-Setup)
- [CLI and Tooling](CLI-and-Tooling)
- [Feature Status](Feature-Status)
- [Runtime Modules](Runtime-Modules)
