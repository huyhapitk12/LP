
## 2026-03-17 - [Precomputing Lengths in Hot Paths]
**Learning:** During lexical analysis (`lexer.c`), avoiding redundant string length recalculations (`strlen`) by precomputing the lengths of predefined keywords and integrating them directly into data structures (like `KW` struct in `check_keyword`) reduces CPU cycles and prevents performance bottlenecks in the compiler frontend.
**Action:** When scanning or processing strings in high-frequency functions like `lexer_next`, always use precalculated lengths (e.g., from AST token structs or embedded table structs) instead of running O(N) string operations like `strlen`.
