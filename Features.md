# LP Language Features Overview

> Last updated: March 2025 | Version 0.3.0

This page provides a comprehensive overview of all LP language features.

## ✅ Fully Supported Features

### Core Language

| Feature | Status | Notes |
|---------|--------|-------|
| Variables & Type Inference | ✅ | Full support |
| Functions | ✅ | Including default args, *args |
| Classes | ✅ | With inheritance |
| `private`/`protected` | ✅ | Compile-time checked |
| `if`/`elif`/`else` | ✅ | |
| `for` loops | ✅ | With `range()` |
| `while` loops | ✅ | With `break`, `continue` |
| `try`/`except`/`finally` | ✅ | |
| `raise` | ✅ | |
| `with` statement | ✅ | |
| Lambda (single-line) | ✅ | |
| Lambda (multiline) | ✅ | |
| String methods | ✅ | Full set |
| F-strings | ✅ | `f"value: {x}"` |
| Pattern Matching | ✅ | `match`/`case` |
| Decorators | ✅ | `@settings`, `@security` |

### Collections

| Feature | Status | Notes |
|---------|--------|-------|
| List literals | ✅ | |
| Dict literals | ✅ | |
| Set literals | ✅ | |
| Tuple literals | ✅ | |
| List comprehensions | ⚠️ | Numeric-focused |
| Dictionary comprehensions | ✅ | `{k: v for x in iter}` |

### Parallel Computing

| Feature | Status | Notes |
|---------|--------|-------|
| `parallel for` | ✅ | Requires OpenMP |
| `@settings` decorator | ✅ | Threads, schedule, GPU |
| GPU device selection | ✅ | `device="gpu"` |
| Unified GPU memory | ✅ | `unified=True` |

### Security Features

| Feature | Status | Notes |
|---------|--------|-------|
| `@security` decorator | ✅ | Rate limiting, auth |
| Input validation | ✅ | `validate=True` |
| SQL injection prevention | ✅ | `prevent_injection=True` |
| XSS prevention | ✅ | `prevent_xss=True` |
| Access control | ✅ | `admin=True`, `readonly=True` |

## ⚠️ Partially Supported

| Feature | Status | Notes |
|---------|--------|-------|
| `thread.spawn` | ⚠️ | Strict worker restrictions |
| Typed exceptions | ⚠️ | Generic runtime behavior |
| Async/await | ⚠️ | Syntactic sugar only - synchronous execution |
| Type unions | ⚠️ | Parsed but no runtime type checking (compiled as `LpVal`) |
| Generic types | ⚠️ | Parsed but no type specialization (no monomorphization) |

### Type System Limitations

**Type Unions (`int | str`):**
- ✅ Syntax is fully parsed into AST (`NODE_TYPE_UNION`)
- ✅ Works for documentation purposes
- ⚠️ Compiled to generic `LpVal` without type constraints
- ❌ No runtime type verification
- ❌ No automatic pattern matching

**Generic Types (`Box[T]`):**
- ✅ Syntax is fully parsed into AST (`NODE_GENERIC_INST`)
- ✅ Type parameters can be declared
- ⚠️ No type specialization at compile time
- ❌ `Box[int]` and `Box[str]` produce identical runtime code
- ❌ No generic type constraint checking

## ✅ Recently Added (v0.3.0)

| Feature | Status | Notes |
|---------|--------|-------|
| `http.post()` | ✅ | Both GET and POST now supported |
| F-strings | ✅ | `f"value: {x}"` |
| Pattern Matching | ✅ | `match`/`case` with guards |
| Decorators | ✅ | `@settings`, `@security` |
| Dictionary comprehensions | ✅ | `{k: v for x in iter}` |
| Generators/Yield | ✅ | Basic generator pattern |
| Type Unions | ⚠️ | `int \| str` parsed, no runtime check |
| Generic Types | ⚠️ | `Box[T]` parsed, no specialization |

## 🎯 Decorators Reference

### @settings Options

```lp
@settings(threads=4)                    # Thread count
@settings(schedule="dynamic", chunk=100) # Scheduling
@settings(device="gpu", gpu_id=0)       # GPU execution
@settings(device="auto")                # Auto-select
@settings(unified=True)                 # Unified memory
```

### @security Options

```lp
@security(level=3)                      # Security level 0-4
@security(auth=True)                    # Require auth
@security(rate_limit=100)               # Requests/min
@security(admin=True)                   # Admin only
@security(readonly=True)                # Read-only
@security(validate=True, sanitize=True) # I/O handling
```

## 📦 Built-in Modules

| Module | Description | Status |
|--------|-------------|--------|
| `math` | Mathematical functions | ✅ |
| `random` | Random number generation | ✅ |
| `time` | Time and date | ✅ |
| `os` | Operating system | ✅ |
| `sys` | System information | ✅ |
| `string` | String utilities | ✅ |
| `http` | HTTP client (GET, POST, PUT, DELETE, PATCH) | ✅ |
| `json` | JSON parsing | ✅ |
| `sqlite` | SQLite database | ✅ |
| `thread` | Threading | ✅ |
| `memory` | Memory management | ✅ |
| `platform` | Platform info | ✅ |
| `numpy` | Array operations (full suite) | ✅ |

## 🔗 See Also

- [Language Reference](Language-Reference)
- [Parallel Computing](Parallel-Computing)
- [Runtime Modules](Runtime-Modules)
