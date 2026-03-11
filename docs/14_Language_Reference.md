# Language Reference

## What you'll learn

This file is a compact lookup reference for LP syntax, builtins, module surface, and command summary.

## Prerequisites

None.

## Core syntax

### Comments

```lp
# comment
```

### Variable declaration and inference

```lp
x: int = 42
name = "LP"
```

### Function definition

```lp
def add(a: int, b: int) -> int:
    return a + b
```

### Class definition

```lp
class Player(Entity):
    health: int = 100
```

### Control flow

```lp
if cond:
    pass
elif other:
    pass
else:
    pass

for i in range(5):
    print(i)

while cond:
    break
```

### Exceptions

```lp
try:
    risky()
except:
    print("handled")
finally:
    print("cleanup")

raise "error"
```

### File-oriented `with`

```lp
with open("file.txt", "r") as f:
    print(f.read())
```

### Lambda

```lp
add = lambda x, y: x + y
```

Multiline form:

```lp
mul = lambda x, y:
    result: int = x * y
    return result
```

### Concurrency

```lp
parallel for i in range(1000):
    work(i)
```

## Access modifiers

- public: default
- `private`
- `protected`

## Builtins

Common builtins documented for normal use:

- `print(x)`
- `len(x)`
- `range(stop)`
- `range(start, stop)`
- `range(start, stop, step)`
- `open(path, mode)`
- `str(x)`
- `int(x)`
- `float(x)`
- `bool(x)`

## Literal forms

- integers: `1`
- floats: `3.14`
- booleans: `true`, `false`
- string: `"text"`
- null-like value: `None`
- list: `[1, 2, 3]`
- dict: `{"k": 1}`
- set: `{1, 2, 3}`
- tuple: `(1, 2)`

## String methods

Supported method-style calls:

- `upper()`
- `lower()`
- `strip()`
- `lstrip()`
- `rstrip()`
- `find(sub)`
- `replace(old, new)`
- `count(sub)`
- `split(delim)`
- `join(parts)`
- `startswith(prefix)`
- `endswith(suffix)`
- `isdigit()`
- `isalpha()`
- `isalnum()`

## Collection notes

- nested subscript such as `rows[0]["n"]` is supported and verified
- list comprehensions exist but are currently documented as partial and numeric-focused
- dictionary comprehensions are not documented as supported public syntax today

## Runtime modules

### `math`

Representative API:

- `pi`, `e`, `tau`
- `sin`, `cos`, `tan`
- `sqrt`, `pow`
- `factorial`, `gcd`, `lcm`

### `random`

- `seed`
- `random`
- `randint`
- `uniform`

### `time`

- `time`
- `sleep`

### `os`

- `getcwd`
- `remove`
- `rename`
- `mkdir`
- `sep`
- `name`

### `os.path`

- `join`
- `exists`
- `isfile`
- `isdir`
- `basename`
- `dirname`
- `getsize`

### `sys`

- `platform`
- `maxsize`
- `exit`
- `getrecursionlimit`

### `http`

- supported: `get`
- unsupported: `post`

### `json`

- supported: `loads`, `dumps`
- unsupported: `parse`

### `sqlite`

- `connect`
- `execute`
- `query`

### `thread`

- `spawn`
- `join`
- `lock_init`
- `lock_acquire`
- `lock_release`

### `memory`

- `arena_new`
- `arena_alloc`
- `arena_reset`
- `arena_free`
- `pool_new`
- `pool_alloc`
- `pool_free`
- `pool_destroy`
- `cast`

### `platform`

- `os`
- `arch`
- `cores`

## Unsupported or partial public surface

- `http.post(...)`: unsupported
- `json.parse(...)`: unsupported
- typed exception objects: partial syntax, generic runtime behavior
- `yield`: reserved but not documented as public syntax yet
- dictionary comprehensions: not documented as supported public syntax yet
- `parallel for`: syntax is supported, runtime acceleration depends on toolchain OpenMP configuration

## CLI summary

```text
lp
lp file.lp
lp file.lp -o out.c
lp file.lp -c out.exe
lp file.lp -asm out.s
lp test [dir]
lp profile file.lp
lp watch file.lp
lp build file.lp [flags]
lp package file.lp --format zip|tar.gz
lp export file.lp [-o name] [--library]
```

## Common errors and limitations

- If a feature is not listed here as supported, assume it needs confirmation before treating it as stable public surface.
- Cross-target builds need matching external toolchains.
- C API export currently has Windows-oriented shared-library naming in the implementation.

## See also

- [Documentation Map](00_Documentation_Map.md)
- [Runtime Modules](08_Runtime_Modules.md)
- [CLI and Tooling](09_CLI_and_Tooling.md)
- [Troubleshooting](13_Troubleshooting.md)
