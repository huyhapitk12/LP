#!/bin/bash
cd /d/LP
echo "=== Starting compilation ==="
/ucrt64/bin/gcc -std=c99 -O2 -Wall \
    compiler/src/main.c \
    compiler/src/lexer.c \
    compiler/src/ast.c \
    compiler/src/parser.c \
    compiler/src/codegen.c \
    -I compiler/src \
    -I runtime \
    -o build/lpc.exe -lm 2>&1
echo "=== GCC exit code: $? ==="
ls -la build/lpc.exe 2>&1
