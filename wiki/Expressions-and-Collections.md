# Expressions and Collections

## What you'll learn

This guide covers string operations, list, dict, set, and tuple literals, indexing and nested subscript access, lambda expressions, and the current collection-comprehension limitations.

## Prerequisites

- Read [Language Basics](Language-Basics) first.

## Syntax And Concepts

### Strings

LP strings support concatenation and common Python-like methods.

Examples of supported string methods:

- `upper()`
- `lower()`
- `strip()`, `lstrip()`, `rstrip()`
- `find(sub)`
- `replace(old, new)`
- `count(sub)`
- `split(delim)`
- `join(parts)`
- `startswith(prefix)`
- `endswith(suffix)`
- `isdigit()`, `isalpha()`, `isalnum()`

String equality uses content comparison.

### Lists, dicts, tuples, and experimental sets

Literal forms:

```lp
numbers = [1, 2, 3]
record = {"name": "LP", "year": 2026}
# Experimental in the current release
flags = {1, 2, 3}
pair = (10, 20)
```

### Indexing and nested subscript

Common access patterns:

```lp
name = record["name"]
first = numbers[0]
```

Nested subscript such as `rows[0]["n"]` is an important verified path in the SQLite tests.

### Lambda expressions

LP supports both single-expression lambdas and multiline lambdas.

Single-expression style:

```lp
add = lambda x, y: x + y
```

Multiline style:

```lp
multiplier = lambda x, y:
    result: int = x * y
    return result
```

### Collection comprehensions

Current status:

- List comprehension syntax exists.
- The current code generation path is numeric and array-oriented.
- Dictionary comprehensions are not documented as supported public syntax today.

Treat comprehensions as a partial feature unless you have verified the exact shape you need.

## Examples

### String methods

```lp
def main():
    s = "  AbC AbC  "
    print(s.strip())
    print(s.lower().strip())
    print(str(s.find("AbC")))
    print(str(s.count("AbC")))

main()
```

### Collection literals and indexing

```lp
def main():
    nums = [10, 20, 30]
    item = {"name": "LP", "ok": true}
    print(nums[1])
    print(str(item["name"]))

main()
```

### Nested subscript from structured data

```lp
import json

def main():
    data = json.loads('{"rows":[{"n":2}]}')
    rows = data["rows"]
    value = int(rows[0]["n"])
    print(value)

main()
```

### Multiline lambda

```lp
def main():
    multiplier = lambda x, y:
        result: int = x * y
        return result

    print(multiplier(10, 5))

main()
```

## Common errors and limitations

- `json.loads(...)` returns a generic `LpVal`-style value, so nested indexing often needs `int(...)`, `str(...)`, `float(...)`, or `bool(...)` casts.
- String operations are well covered by the regression tests, but collection comprehensions are not yet documented as a fully general public feature.
- Dictionary comprehensions are not part of the verified public language surface in the current parser/codegen path.
- String `split()` returns an internal string-array representation that is intended to work with `join()` and indexing-like usage patterns, not as a full Python list clone.

## See also

- [Language Basics](Language-Basics)
- [Runtime Modules](Runtime-Modules)
- [Language Reference](Language-Reference)
