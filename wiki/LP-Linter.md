# LP Linter

LP Linter is a static analysis tool for LP language, similar to **pylint**, **flake8**, **mypy** for Python.

## 🚀 Basic Usage

```bash
# Check LP file
python tools/lp_linter.py myfile.lp

# Or use wrapper script
./tools/lp-check myfile.lp
```

### Options

| Flag | Description |
|------|-------------|
| `--strict` | Enable strict mode (check style) |
| `--json` | Output results as JSON |
| `--quiet` | Show only errors (no warnings) |
| `--no-color` | Disable colored output |

### Examples

```bash
# Strict mode
python tools/lp_linter.py myfile.lp --strict

# JSON output
python tools/lp_linter.py myfile.lp --json

# Only errors
python tools/lp_linter.py myfile.lp --quiet
```

## 🔍 Errors Detected

### Errors (E001-E009)

| Code | Description | Example |
|------|-------------|---------|
| E001 | Unknown module | `import unknown_module` |
| E002 | Unknown type | `x: foobar = 1` |
| E003 | Missing type annotation | `def f(x): pass` |
| E004 | Using `None` instead of `null` | `x = None` |
| E005 | Type `ptr` not allowed | `x: ptr = ...` |
| E006 | Missing `:` after keyword | `if x > 5` |
| E007 | Common typo | `printl("hi")` |
| E008 | Missing `dsa.flush()` | When using `dsa.write_*` |
| E009 | DSA output without flush | When using dsa module |

### Warnings (W001-W002)

| Code | Description |
|------|-------------|
| W001 | Assignment in if (did you mean `==`?) |
| W002 | Function defined but never called |

### Style (S001-S003) - only with `--strict`

| Code | Description |
|------|-------------|
| S001 | Line too long (> 100 chars) |
| S002 | Trailing whitespace |
| S003 | Multiple spaces around operator |

## 📝 Output Examples

### Normal output

```
🔍 Found 3 issue(s): 2 error(s), 1 warning(s), 0 style

❌ [E004] Line 5:7: Use 'null' instead of 'None' in LP
    💡 Suggestion: Replace 'None' with 'null'

❌ [E008] Line 10:1: dsa module requires dsa.flush() before program exit
    💡 Suggestion: Add 'dsa.flush()' at the end of your program

⚠️ [W002] Line 3:1: Function 'main/solve' defined but never called
    💡 Suggestion: Add 'main()' or 'solve()' at the end of the file
```

### JSON output

```json
[
  {
    "line": 5,
    "col": 7,
    "code": "E004",
    "severity": "ERROR",
    "message": "Use 'null' instead of 'None' in LP",
    "suggestion": "Replace 'None' with 'null'"
  }
]
```

## ✅ Correct Code

```lp
import dsa

def solve() -> int:
    n: int = dsa.read_int()
    dsa.write_int_ln(n * 2)
    dsa.flush()  # ← Correct: has flush()
    return 0

solve()  # ← Correct: calls function
```

## ❌ Incorrect Code

```lp
import dsa

def solve() -> int:
    n: int = dsa.read_int()
    arr: list = None  # ❌ E004: Using None
    dsa.write_int_ln(n * 2)
    # ❌ E008: Missing dsa.flush()
    return 0

# ❌ W002: Not calling solve()
```

## 🔧 Editor Integration

### VS Code

Add to `settings.json`:

```json
{
    "files.associations": {
        "*.lp": "lp"
    },
    "terminal.integrated.commandsToRun": [
        "python tools/lp_linter.py ${file}"
    ]
}
```

### Vim/Neovim

```vim
" Add to .vimrc
autocmd BufWritePost *.lp :!python tools/lp_linter.py %
```

### Pre-commit Hook

Create file `.git/hooks/pre-commit`:

```bash
#!/bin/bash
for file in $(git diff --cached --name-only | grep '\.lp$'); do
    python tools/lp_linter.py "$file" --quiet
    if [ $? -ne 0 ]; then
        echo "Linter found errors in $file"
        exit 1
    fi
done
```

## 🎯 Best Practices

1. **Always run linter before compiling**
   ```bash
   python tools/lp_linter.py myfile.lp && lp myfile.lp
   ```

2. **Use `--strict` for production code**
   ```bash
   python tools/lp_linter.py myfile.lp --strict
   ```

3. **Integrate into CI/CD**
   ```yaml
   # GitHub Actions
   - name: Lint LP files
     run: python tools/lp_linter.py src/*.lp --strict
   ```

## 📊 Comparison with Python

| Python | LP |
|--------|-----|
| `pylint file.py` | `python tools/lp_linter.py file.lp` |
| `flake8 file.py` | `python tools/lp_linter.py file.lp --strict` |
| `mypy file.py` | Type checking built into linter |
| `black file.py` | No formatter yet |

## 🔄 Recommended Workflow

```bash
# 1. Write code
vim solution.lp

# 2. Run linter
python tools/lp_linter.py solution.lp --strict

# 3. Fix errors

# 4. Compile
lp solution.lp -c solution

# 5. Test
./solution < input.txt

# 6. Commit
git add solution.lp
git commit -m "Add solution"
```
