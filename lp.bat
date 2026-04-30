@echo off
setlocal disableDelayedExpansion

REM Thêm MSYS2 vào PATH để tìm GCC
set "PATH=C:\msys64\ucrt64\bin;%PATH%"

REM Nếu cần build
if "%~1"=="--build" (
    shift
    goto :DoBuild
)

REM Nếu app chưa build, tự động gọi build
if not exist "%~dp0build\lp.exe" (
    echo [LP] Compiler chua duoc build! Dang tu tao build\lp.exe...
    call :DoBuild
    if ERRORLEVEL 1 exit /b 1
)

REM Chạy code của user
"%~dp0build\lp.exe" %* 2>&1
exit /b %ERRORLEVEL%

:DoBuild
echo [LP Build] Compiling LP compiler...
if not exist "%~dp0build" mkdir "%~dp0build"
set "CFLAGS=-std=c11 -O2 -Wall -Wextra"

set "GCC="
for /f "delims=" %%I in ('where gcc 2^>nul') do (
    set "GCC=%%I"
    goto :gcc_found
)
echo [LP Build] Error: No C compiler found! Install MSYS2.
exit /b 1

:gcc_found
echo [LP Build] Using GCC: %GCC%
"%GCC%" %CFLAGS% ^
    "%~dp0compiler\src\main.c" "%~dp0compiler\src\lexer.c" "%~dp0compiler\src\ast.c" ^
    "%~dp0compiler\src\parser.c" "%~dp0compiler\src\codegen.c" "%~dp0compiler\src\codegen_asm.c" ^
    "%~dp0compiler\src\asm_optimize.c" "%~dp0compiler\src\repl.c" "%~dp0compiler\src\process_utils.c" ^
    "%~dp0compiler\src\error_reporter.c" "%~dp0compiler\src\semantic_check.c" "%~dp0compiler\src\type_inference.c" ^
    -I "%~dp0compiler\src" -I "%~dp0runtime" -o "%~dp0build\lp.exe" -lm

if %ERRORLEVEL% NEQ 0 (
    echo [LP Build] Compilation failed!
    exit /b 1
)

"%GCC%" -O2 -c "%~dp0runtime\sqlite3.c" -o "%~dp0runtime\sqlite3.o"
if %ERRORLEVEL% NEQ 0 (
    echo [LP Build] Failed to build runtime\sqlite3.o
    exit /b 1
)

echo [LP Build] Success: build\lp.exe
exit /b 0
