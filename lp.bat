@echo off
REM LP Language Runner
REM Usage: lp file.lp  or  lp file.py
set "PATH=C:\msys64\ucrt64\bin;%PATH%"
"%~dp0build\lp.exe" %* 2>&1
exit /b %ERRORLEVEL%
