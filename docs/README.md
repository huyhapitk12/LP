# LP Language Documentation
Welcome to the Official Guide for the LP Programming Language. LP is a statically-typed, ahead-of-time (AOT) compiled language structurally identical to Python. While looking like Python, it translates flawlessly down to Native C, avoiding global lock bottlenecks (No GIL) and offering native low-level pointers control when you need it!

## Docs Index
1. **[01_Getting_Started.md](01_Getting_Started.md)** - CLI Commands, setup and first program.
2. **[02_Syntax_and_Semantics.md](02_Syntax_and_Semantics.md)** - Basic types, variable inferences, conditionals, and iterators.
3. **[03_Object_Oriented.md](03_Object_Oriented.md)** - Classes, Inheritance, Polymorphism.
4. **[04_Memory_Management.md](04_Memory_Management.md)** - Arenas, Pools, and Raw Memory Allocation logic.
5. **[05_Multithreading.md](05_Multithreading.md)** - High-performance POSIX-Threading interfaces (`spawn`, `join`, `lock`).
6. **[06_Standard_Library.md](06_Standard_Library.md)** - Core functionalities (OS, SYS, String, SQLite, JSON, HTTP).
