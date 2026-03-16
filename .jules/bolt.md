## 2024-03-16 - [Lexer Performance: Avoid runtime strlen for static keywords]
**Learning:** In `lexer.c`, `check_keyword` repeatedly computes the length of statically known keywords using `strlen()` during lexical analysis, which incurs significant overhead in the parser.
**Action:** Replace `strlen()` with precomputed string lengths added directly to the keyword struct array, avoiding runtime length calculations in hot execution paths.
