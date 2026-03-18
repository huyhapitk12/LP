# C API Export

## What you'll learn

This guide explains how `lp export` generates a C-facing header, how `--library` extends that flow, which LP functions become part of the exported API, and which limitations still matter today.

## Prerequisites

- Read [CLI and Tooling](CLI-and-Tooling) first.
- A working host C compiler is required if you use `--library`.

## Concepts

`lp export` is aimed at interoperability with native C or C-compatible callers.

Current public workflow:

- parse an LP file
- generate a C API header
- optionally compile a shared library artifact

## Header-only export

Generate a header:

```bash
lp export api.lp -o demo_api
```

Result:

- `demo_api.h`

If `-o` is omitted, LP uses the LP file basename.

## Header plus shared library

Generate both:

```bash
lp export api.lp --library -o demo_api
```

Result in the current implementation:

- `demo_api.h`
- `demo_api.dll`

## What gets exported

The header generator walks top-level LP function definitions.

Current rules:

- top-level public functions are exported
- `private` top-level functions are skipped
- top-level function names starting with `_` are skipped by convention
- classes are not automatically turned into a documented stable C object API here

## Naming convention

Exported symbols are prefixed with `lp_`.

If your LP function is:

```lp
def add(a: int, b: int) -> int:
    return a + b
```

The generated declaration follows this pattern:

```c
LP_EXTERN int64_t lp_add(int64_t lp_a, int64_t lp_b);
```

## Example LP source

```lp
def add(a: int, b: int) -> int:
    return a + b
```

Generate the API:

```bash
lp export add.lp -o add_api
lp export add.lp --library -o add_api
```

## Minimal C integration example

```c
#include "add_api.h"
#include <stdio.h>

int main(void) {
    printf("%lld\n", (long long)lp_add(20, 22));
    return 0;
}
```

## Important implementation details

- The generated header uses `__declspec(dllexport)` on Windows and visibility attributes elsewhere.
- The current library artifact naming is still `.dll` in the implementation when `--library` is requested.
- Parameters without explicit LP type annotations may degrade to less helpful C signatures in exported headers. For stable native interop, prefer explicit parameter and return annotations.

## Common errors and limitations

- `--library` requires a working host C compiler and the runtime include path.
- The current implementation is oriented around C-callable top-level functions, not full class or object export.
- Shared-library naming is not yet normalized across Windows, Linux, and macOS.
- Experimental Python-interoperability paths are unrelated to `lp export` and are not a replacement for this stable header-generation path.

## See also

- [CLI and Tooling](CLI-and-Tooling)
- [Build and Distribution](Build-and-Distribution)
- [Troubleshooting](Troubleshooting)
