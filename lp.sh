#!/bin/bash
set -e

SCRIPT_DIR="${0%/*}"
[ "$SCRIPT_DIR" = "$0" ] && SCRIPT_DIR="."
cd "$SCRIPT_DIR"

if [ -d /ucrt64/bin ] && [ -d /usr/bin ]; then
    export PATH="/ucrt64/bin:/usr/bin:$PATH"
fi

OUT="build/lp"
case "$(uname -s 2>/dev/null)" in
    CYGWIN*|MINGW*|MSYS*) OUT="build/lp-posix.exe" ;;
esac

do_build() {
    echo "=== Starting compilation ==="
    mkdir -p build
    WARN_FLAGS="-Wall -Wextra"
    gcc -std=gnu99 -O2 $WARN_FLAGS \
        compiler/src/main.c compiler/src/lexer.c compiler/src/ast.c \
        compiler/src/parser.c compiler/src/codegen.c compiler/src/codegen_asm.c \
        compiler/src/asm_optimize.c compiler/src/repl.c compiler/src/process_utils.c \
        compiler/src/error_reporter.c compiler/src/semantic_check.c \
        -I compiler/src -I runtime -o "$OUT" -lm

    gcc -std=gnu99 -O2 -c runtime/sqlite3.c -o runtime/sqlite3.o
    echo "=== Build successful: $OUT ==="
}

if [ "$1" = "--build" ]; then
    do_build
    exit 0
fi

if [ ! -f "$OUT" ]; then
    echo "Compiler not found via $OUT. Auto building..."
    do_build
fi

if [ $# -eq 0 ]; then
    "$OUT"
else
    "$OUT" "$@"
fi
