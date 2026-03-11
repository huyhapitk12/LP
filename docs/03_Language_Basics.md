# Language Basics

## What you'll learn

This guide covers LP variables, literals, type annotations, type inference, operators, control flow, and functions.

## Prerequisites

- Read [First Programs](02_First_Programs.md) first.

## Syntax And Concepts

### Comments

LP uses Python-style line comments:

```lp
# this is a comment
```

### Variables and literals

Common literal forms:

```lp
count = 21
ratio = 3.14
enabled = true
name = "LP"
empty = None
```

### Type annotations

You can annotate variables and parameters explicitly:

```lp
count: int = 21
ratio: float = 3.14
name: str = "LP"
```

### Type inference

LP also supports inference for many common assignments:

```lp
n = 21
text = "LP"
```

This is already used by the regression suite in `examples/test_type_inference.lp`.

### Operators

Common operators include:

- arithmetic: `+`, `-`, `*`, `/`, `//`, `%`, `**`
- comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- boolean: `and`, `or`, `not`

For strings, `==` and `!=` compare content, not pointer identity.

### Control flow

LP supports standard block-based control flow:

```lp
if score > 90:
    print("A")
elif score > 80:
    print("B")
else:
    print("C")
```

Loop forms:

```lp
for i in range(5):
    print(i)

while ready:
    break
```

Useful flow keywords:

- `return`
- `pass`
- `break`
- `continue`

### Functions

Functions use Python-like syntax:

```lp
def twice(n: int) -> int:
    return n * 2
```

Return annotations are optional in the current parser:

```lp
def twice(n: int):
    return n * 2
```

## Examples

### Variables and inference

```lp
def main():
    n = 21
    text = "LP"
    print(n)
    print(text)

main()
```

### Control flow and loops

```lp
def main():
    total: int = 0
    for i in range(5):
        total = total + i
    if total == 10:
        print("ok")
    else:
        print("bad")

main()
```

### Functions

```lp
def twice(n: int) -> int:
    return n * 2

def main():
    print(twice(21))

main()
```

## Common errors and limitations

- A missing colon is a common parser error after `def`, `if`, `elif`, `else`, `for`, `while`, `try`, and `class`.
- LP expects indentation for block bodies. If you want an empty block, use `pass`.
- Type inference works well for common local assignments, but explicit annotations are still useful when you want a stable public API or clearer compiler intent.
- Default parameter values are parsed, but this guide focuses on the most verified function forms used by the current examples and tests.

## See also

- [Expressions and Collections](04_Expressions_and_Collections.md)
- [Object-Oriented Programming](05_Object_Oriented_Programming.md)
- [Language Reference](14_Language_Reference.md)
