## 2024-05-24 - [Lexer optimization: Keyword matching]
**Learning:** Checking keywords against a linear array of strings with memcmp is a measurable performance bottleneck in LP's Lexing phase (lexer.c).
**Action:** Instead of linearly comparing against all possible keywords, we can switch on the length of the string to instantly reduce the number of potential keyword candidates to just a handful, significantly speeding up tokenization.
