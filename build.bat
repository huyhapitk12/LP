@echo off
setlocal
REM LP Compiler Build Script
REM Usage: build.bat

echo [LP Build] Compiling LP compiler...
if not exist "build" mkdir build
set "CFLAGS=-std=c11 -O2 -Wall -Wextra"
set "GCC="

REM Prefer GCC already exposed in PATH (GitHub Actions adds this for us)
for /f "delims=" %%I in ('where gcc 2^>nul') do (
    set "GCC=%%I"
    goto :gcc_found
)

REM Fallback for local MSYS2 installs
if exist "C:\msys64\ucrt64\bin\gcc.exe" (
    set "PATH=C:\msys64\ucrt64\bin;%PATH%"
    for /f "delims=" %%I in ('where gcc 2^>nul') do (
        set "GCC=%%I"
        goto :gcc_found
    )
)

echo [LP Build] Error: No C compiler found! Install MSYS2.
exit /b 1

:gcc_found
echo [LP Build] Using GCC: %GCC%
"%GCC%" %CFLAGS% ^
    compiler\src\main.c ^
    compiler\src\lexer.c ^
    compiler\src\ast.c ^
    compiler\src\parser.c ^
    compiler\src\codegen.c ^
    compiler\src\repl.c ^
    compiler\src\process_utils.c ^
    -I compiler\src ^
    -I runtime ^
    -o build\lp.exe -lm
if %ERRORLEVEL% NEQ 0 (
    echo [LP Build] Compilation failed!
    exit /b 1
)

"%GCC%" -O2 -c runtime\sqlite3.c -o runtime\sqlite3.o
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
exit /b 0
