# Syntax and Semantics
LP aims for zero-overhead clarity. If you understand standard scripting patterns, you know LP!

## Type Annotations & Inference
Variable declarations can define the base types statically. LP also infers variables natively if left unannotated!
```python
# Unannotated Inference
x = 50           # Assumed INT
name = "Alice"   # Assumed STRING

# Annotated Strict Typing
age: int = 30
weight: float = 75.5
active: bool = True
```

## Basic Mathematical & Logical Constructs
All statements share generic conditional syntaxes evaluating correctly down to primitive architectures:
```python
age = 20

if age >= 18:
    print("Welcome adult!")
else:
    print("Access Denied.")
    
# Iterations
for i in range(10):
    print("Index is:")
    print(i)
```

## Lambda Functions
LP supports true Multiline Lambdas without sacrificing compilation boundaries!
```python
multiplier = lambda x, y:
    res: int = x * y
    return res

print(multiplier(10, 5)) # Outputs 50
```
