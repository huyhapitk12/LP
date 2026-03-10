# LP Language Runner (PowerShell)
# Usage: lp file.lp  or  lp file.py
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
& "$PSScriptRoot\build\lp.exe" @args
