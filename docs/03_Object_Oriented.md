# Object Oriented Programming
LP evaluates struct schemas dynamically enabling Zero-Cost inheritance trees evaluated safely against backend compilation properties!

## Classes and Base Initialization
Class definitions implicitly invoke `__init__` constructor methods natively!
```python
class Animal:
    name = "Unknown"
    def speak(self) -> str:
        return "..."

class Dog(Animal):
    def speak(self) -> str:
        return "Woof!"

def main():
    d = Dog()
    print(d.speak()) # Outputs "Woof!"
```

Due to strict C translations, LP recursively flattens Inherited attributes down resolving exact object bounds dynamically avoiding large generic map evaluations!
