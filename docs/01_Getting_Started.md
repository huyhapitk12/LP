# Getting Started
LP uses an AOT Compilation engine bridging Python-like structure with pure native execution.

### The CLI Tool
To utilize the compiler, direct the `lp` alias to your script:
```bash
# Run a File Immediately
lp file.lp

# Compile to Native OS Executable
lp file.lp -c out.exe

# Compile to Intermediate C
lp file.lp -o out.c

# Compile standalone with GCC toolchains 
lp build file.lp --target windows-x64
lp build file.lp --target linux-x64

# Package the Build into automated zips/tar.bz
lp package file.lp --format zip

# Run Auto-Test suite on a directory
lp test examples/

# Enter Interactive REPL
lp
```

### Your First Program
Write this logic into `hello.lp`:
```python
def main():
    print("Welcome to LP natively!")
    
main()
```
Run it via:
```bash
lp hello.lp
```
