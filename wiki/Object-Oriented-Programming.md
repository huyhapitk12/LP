# Object-Oriented Programming

## What you'll learn

This guide covers classes, fields, methods, inheritance, polymorphism, and LP's compile-time checked `private` and `protected` access rules.

## Prerequisites

- Read [Language Basics](Language-Basics) first.

## Syntax And Concepts

### Declaring a class

LP classes use Python-like syntax with field declarations and methods.

```lp
class Entity:
    x: float = 0.0
    y: float = 0.0

    def move(self, dx: float, dy: float) -> float:
        self.x = self.x + dx
        self.y = self.y + dy
        return self.x
```

### Instantiation

Create an object by calling the class name:

```lp
player = Player()
```

The current examples use default field values rather than custom constructor methods.

### Inheritance

Subclasses can inherit from a base class:

```lp
class Player(Entity):
    health: int = 100
```

### Polymorphism

Methods can be overridden in subclasses. The regression suite verifies inherited and overridden behavior.

### Access control

LP checks member visibility at compile time.

- `private`: accessible only inside the declaring class.
- `protected`: accessible inside the declaring class and its subclasses.
- public: the default when no access modifier is present.

The current compiler enforces these rules for:

- field reads
- field writes
- method calls

## Examples

### Core OOP example

This is the same style used by `tests/regression/02_oop_game_engine.lp`.

```lp
class Entity:
    x: float = 0.0
    y: float = 0.0

    def move(self, dx: float, dy: float) -> float:
        self.x = self.x + dx
        self.y = self.y + dy
        return self.x

class Player(Entity):
    health: int = 100
    name: str = "Hero"

    def take_damage(self, amount: int) -> int:
        self.health = self.health - amount
        return self.health
```

### Inheritance and `protected`

```lp
class Dog:
    protected kind: str = "dog"

class Puppy(Dog):
    def reveal_kind(self) -> str:
        return self.kind
```

This pattern is verified by `tests/regression/test_inheritance_access.lp`.

### `private` rejection example

```lp
class SecretBox:
    private code: int = 7

box = SecretBox()
# box.code = 9
```

The commented line should be treated as a compile-time error in the current compiler.

## Common errors and limitations

- Access control is compile-time enforced. If you try to touch a `private` or `protected` member from the wrong scope, the compiler should reject it.
- `protected` is designed for subclass access, not unrestricted same-file access.
- The public docs focus on field defaults and methods. More elaborate object-construction patterns are possible in source, but they are not the center of the verified example set yet.
- Exported C APIs skip `private` functions and top-level names that start with `_` by convention.

## See also

- [Expressions and Collections](Expressions-and-Collections)
- [Error Handling and Advanced Control Flow]
- [Examples Cookbook](Examples-Cookbook)
- [Language Reference](Language-Reference)
