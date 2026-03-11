@echo off
REM LP Compiler Build Script
REM Usage: build.bat

echo [LP Build] Compiling LP compiler...
if not exist "build" mkdir build
set "CFLAGS=-std=c99 -O2 -Wall -Wextra -Wpedantic"

REM Try MSYS2 UCRT64 GCC
if exist "C:\msys64\ucrt64\bin\gcc.exe" (
    echo [LP Build] Using MSYS2 UCRT64 GCC...
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    gcc %CFLAGS% ^
        compiler\src\main.c ^
        compiler\src\lexer.c ^
        compiler\src\ast.c ^
        compiler\src\parser.c ^
        compiler\src\codegen.c ^
        compiler\src\repl.c ^
        -I compiler\src ^
        -I runtime ^
        -o build\lp.exe -lm
    if %ERRORLEVEL% EQU 0 (
        gcc -O2 -c runtime\sqlite3.c -o runtime\sqlite3.o
        if %ERRORLEVEL% NEQ 0 (
            echo [LP Build] Failed to build runtime\sqlite3.o
            exit /b 1
        )
        echo [LP Build] Success: build\lp.exe
        echo.
        echo Usage:
        echo   lp file.lp            Run directly
        echo   lp file.lp -o out.c   Generate C code
        echo   lp file.lp -c out.exe Compile to executable
    ) else (
        echo [LP Build] Compilation failed!
        exit /b 1
    )
    exit /b 0
)

echo [LP Build] Error: No C compiler found! Install MSYS2.
exit /b 1
