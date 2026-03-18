#!/bin/bash
set -e

SCRIPT_DIR="${0%/*}"
if [ "$SCRIPT_DIR" = "$0" ]; then
    SCRIPT_DIR="."
fi
cd "$SCRIPT_DIR"

if [ -d /ucrt64/bin ] && [ -d /usr/bin ]; then
    export PATH="/ucrt64/bin:/usr/bin:$PATH"
fi

OUT="build/lp"
case "$(uname -s 2>/dev/null)" in
    CYGWIN*|MINGW*|MSYS*) OUT="build/lp-posix.exe" ;;
esac

echo "=== Starting compilation ==="
mkdir -p build
WARN_FLAGS="-Wall -Wextra"
gcc -std=gnu99 -O2 $WARN_FLAGS \
    compiler/src/main.c \
    compiler/src/lexer.c \
    compiler/src/ast.c \
    compiler/src/parser.c \
    compiler/src/codegen.c \
    compiler/src/codegen_asm.c \
    compiler/src/asm_optimize.c \
    compiler/src/repl.c \
    compiler/src/process_utils.c \
    -I compiler/src \
    -I runtime \
    -o "$OUT" -lm

gcc -std=gnu99 -O2 -c runtime/sqlite3.c -o runtime/sqlite3.o

echo "=== Build successful: $OUT ==="
ls -la "$OUT"
ls -la runtime/sqlite3.o
