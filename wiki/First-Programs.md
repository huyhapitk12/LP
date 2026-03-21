# First Programs

## What you'll learn

This guide shows how to run an LP file, use the REPL, generate C output, compile a native executable, and follow a simple edit-run-debug workflow.

## Prerequisites

- The compiler must already build successfully.
- If needed, follow [Installation and Setup](Installation-and-Setup) first.

## Concepts

LP has three everyday workflows:

- Run an LP file directly.
- Generate C for inspection.
- Compile a reusable native executable.

The direct-run path is convenient for iteration. The compile path is better when you want a named artifact. On Windows, prefer the GCC-backed path for predictable verification.

## Run Your First Program

Create `hello.lp`:

```lp
print("Hello from LP")
```

Run it on Windows or whenever you want the most predictable local verification path:

```bash
lp hello.lp --gcc
```

What happens with `--gcc`:

- LP reads and parses `hello.lp`.
- LP generates a temporary C file.
- LP invokes the host C compiler.
- LP runs the resulting temporary executable.

Without `--gcc`, `lp hello.lp` uses the native ASM backend by default.

## Generate C Code

```bash
lp hello.lp -o hello.c
```

Use this when you want to:

- inspect generated C
- debug code generation problems
- compile the output manually with your own toolchain flags

## Compile A Native Executable

```bash
lp hello.lp --gcc -c hello.exe
```

On non-Windows hosts, you would normally use a suffixless name instead:

```bash
lp hello.lp -c hello
```

This path keeps the final artifact instead of deleting it after the run.

## Generate Assembly

```bash
lp hello.lp -asm hello.s
```

Use this when you want to inspect LP's native assembly output.

## Use The REPL

Start the interactive shell:

```bash
lp
```

Useful REPL commands:

- `.help`
- `.clear`
- `.show`
- `.exit`
- `.quit`

Example session:

```text
> x: int = 3
> print(x)
3
```

The REPL accumulates state and recompiles the current session after each accepted input.

## Multi-Line Input In The REPL

You can enter blocks such as functions and control flow:

```text
> def twice(n: int) -> int:
.     return n * 2
.
> print(twice(21))
42
```

The REPL also recognizes block starters such as `class`, `if`, `for`, `while`, `try`, `except`, `finally`, and `with`.

## Edit, Run, Debug

A practical local loop looks like this:

1. Edit `hello.lp`.
2. Run `lp hello.lp --gcc`.
3. If parsing fails, read the contextual compiler error.
4. If you need more detail, run `lp hello.lp -o hello.c`.
5. When the program is stable, run `lp hello.lp --gcc -c hello.exe` or `lp build hello.lp --release --gcc`.

## Examples

### Small function program

```lp
def twice(n: int) -> int:
    return n * 2

print(twice(21))
```

Run it:

```bash
lp hello.lp --gcc
```

### C generation path

```bash
lp hello.lp -o hello.c
```

### Executable path

```bash
lp hello.lp --gcc -c hello.exe
hello.exe
```

## Common errors and limitations

- `lp hello.lp` defaults to the native ASM backend. On Windows, `lp hello.lp --gcc` is the safer path for install verification.
- If you only want parser or codegen output, `-o` is the least demanding path because it stops after generating C.
- The REPL recompiles accumulated state rather than interpreting line-by-line bytecode.
- Direct run mode creates temporary artifacts and cleans up the generated executable after execution.

## See also

- [Language Basics](Language-Basics)
- [CLI and Tooling](CLI-and-Tooling)
- [Troubleshooting](Troubleshooting)
