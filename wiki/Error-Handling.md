# Error Handling

## What you'll learn

This guide covers `try`, `except`, `finally`, `raise`, file-oriented `with`, and how to read LP parser and codegen diagnostics.

## Prerequisites

- Read [Language Basics](Language-Basics) first.

## Syntax And Concepts

### Raising an exception

LP supports string-based `raise` expressions:

```lp
raise "something went wrong"
```

### `try`, `except`, and `finally`

Basic structure:

```lp
try:
    risky_call()
except:
    print("handled")
finally:
    print("cleanup")
```

The current runtime uses a generic exception mechanism. Typed `except SomeType as err:` syntax is parsed, but the current code generation path treats exception handling generically.

### `with` for file handles

LP supports `with` statements around file handles returned by `open()`.

```lp
with open("notes.txt", "w") as f:
    f.write("hello\n")
```

The generated C code closes the file automatically at the end of the block.

### File operations

The current public file surface is:

- `open(path, mode)`
- `file.read()`
- `file.write(data)`
- `file.close()`

### Reading parser diagnostics

LP's parser error output is designed to show:

- the main error message
- nearby source lines
- a highlighted error line
- hints for common mistakes such as missing colons or missing indentation

## Examples

### Raise and catch

```lp
def main():
    try:
        raise "broken"
    except:
        print("handled")
    finally:
        print("done")

main()
```

### `with` and file I/O

```lp
def main():
    with open("notes.txt", "w") as f:
        f.write("LP\n")

    with open("notes.txt", "r") as f:
        text = f.read()

    print(text)

main()
```

### What a syntax failure usually means

```lp
def main()
    print("missing colon")
```

Expected outcome:

- LP should reject the file.
- The compiler should show the offending line and a hint about the missing colon.

## Common errors and limitations

- `except SomeType as err:` is parsed, but current exception handling is generic. Do not depend on typed exception objects yet.
- `with` is currently documented for file handles from `open()`. Treat arbitrary context-manager semantics as unsupported.
- `yield` exists in the lexer and AST, but it is not documented as a stable public language feature yet.
- If you get `expected indented block`, add an indented body or use `pass`.
- If you get `expected ':'`, inspect the end of the preceding control-flow or declaration line.

## See also

- [Runtime Modules](Runtime-Modules)
- [CLI and Tooling](CLI-and-Tooling)
- [Troubleshooting](Troubleshooting)
- [Language Reference](Language-Reference)
