## 2024-05-24 - [Avoid dynamic allocation per AST Node]
**Learning:** Calling `calloc` and `free` dynamically for thousands of AST nodes creates fragmentation and overhead in parsing. Additionally, when using an arena allocator, we must ensure the `free()` calls are completely stripped, as attempting to `free` an arena-managed pointer will trigger segmentation faults and invalid free errors.
**Action:** Switch to `LpArena` for AST node allocations inside `parser.c` and `ast.c`, and systematically remove all `free()` calls matching these nodes.
