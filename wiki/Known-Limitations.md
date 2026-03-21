# Known Limitations

This page consolidates all known limitations in the current LP release. Check here before filing a bug report — many of these are documented design constraints, not bugs.

For feature implementation status, see [Feature Status](Feature-Status).

---

## Language Runtime

### `thread.spawn` — zero-argument workers only

The `thread` module currently requires workers to be named LP functions with **zero arguments** returning `int` or `void`.

```lp
# ✅ Supported
def worker() -> int:
    return 42

t = thread.spawn(worker)
result = thread.join(t)   # result = 42

# ❌ Not supported — no argument passing
t = thread.spawn(worker, 42)

# ❌ Not supported — lambda worker
t = thread.spawn(lambda: 42)

# ❌ Not supported — wrong return type
def bad_worker() -> str:
    return "hello"
```

**Workaround:** Use a global variable or module-level state to pass data to the worker.

---

### Typed exceptions — generic runtime dispatch

`except SomeType as err:` syntax is parsed and compiles, but exception dispatch at runtime is generic. All exceptions are caught by any `except` block regardless of type.

```lp
# ✅ Works — generic catch
try:
    raise "something went wrong"
except:
    print("caught")

# ⚠️ Parsed but not type-dispatched at runtime
try:
    raise "network error"
except NetworkError as e:   # catches ALL exceptions, not just NetworkError
    print("caught")
```

---

### async/await — synchronous execution only

`async def` and `await` are syntactic sugar. Code runs **synchronously** — there is no event loop, no async I/O, no true concurrency from `await`.

```lp
# ⚠️ This runs synchronously — fetch_data blocks until complete
async def fetch_data(url: str) -> str:
    return http.get(url)

async def main():
    data = await fetch_data("https://example.com")  # blocks here
    print(data)
```

---

### Set literals — experimental

Set literal syntax `{1, 2, 3}` exists but is not stable in the current release. Avoid in production code.

```lp
# ⚠️ May not work reliably
unique = {1, 2, 3, 2, 1}

# ✅ Use list + manual dedup instead
items = [1, 2, 3, 2, 1]
```

---

### List comprehensions — numeric-focused only

Simple numeric list comprehensions work. Filter conditions (`if`) and complex expressions are partially supported.

```lp
# ✅ Supported
squares = [i * i for i in range(10)]

# ⚠️ Partially supported — may not work for all cases
filtered = [x for x in data if x > 0]

# ⚠️ Not supported — dict comprehensions require v0.1.1+
squares_dict = {x: x*x for x in range(10)}
```

---

### Generic types — no type specialization

`class Box[T]` syntax parses correctly, but `Box[int]` and `Box[str]` compile to the same runtime type. No monomorphization or generic type checking.

```lp
class Box[T]:
    value: T

# Both compile to identical runtime code — no type safety
int_box: Box[int] = Box()
str_box: Box[str] = Box()
int_box.value = "hello"  # No error — type not enforced at runtime
```

---

### Type unions — no runtime checking

`int | str` type annotations parse and compile but do not enforce type safety at runtime. All union types compile to the generic `LpVal`.

```lp
def process(x: int | str) -> int | str:
    return x

process(3.14)  # No error — float accepted despite annotation
```

---

## Compiler Backends

### ASM backend — Linux/macOS only (stable)

The native ASM backend (default `lp file.lp`) assumes a Linux-style assembler/linker toolchain (`as` + `ld`). On Windows it may fail with assembler errors.

**On Windows, always use:**
```bash
lp file.lp --gcc
lp build file.lp --release --gcc
```

---

### Cross-target builds — requires matching toolchain

`lp build --target linux-x64` requires the matching cross-compiler (`x86_64-linux-gnu-gcc`) to be installed. LP reports the missing tool clearly, but the build will fail if the toolchain is absent.

---

## Output and I/O

### `dsa.write_float` — fixed 4 decimal places

`dsa.write_float(x)` always outputs 4 decimal places. For custom precision, use:

```lp
dsa.write_float_prec(x, 4)     # same as write_float
dsa.write_float_prec(x, 6)     # 6 decimal places
dsa.write_float_prec_ln(x, 4)  # with newline
```

---

### Missing `dsa.flush()` causes no output

All `dsa.write_*` functions buffer output. **Always call `dsa.flush()` before program exit** or output will not appear.

```lp
def main():
    dsa.write_int_ln(42)
    dsa.flush()  # REQUIRED
```

---

## C API Export

### `lp export --library` — Windows `.dll` naming only

`lp export api.lp --library` currently generates `.dll` even on Linux/macOS. Cross-platform shared library naming (`.so`, `.dylib`) is planned but not yet implemented.

---

## Performance

### LpVal overhead — all values are tagged unions

Every LP value is a 16-byte tagged union (`LpVal`), even when the type is known at compile time. This causes ~2× overhead vs C++ for numeric-heavy code. See [Performance Guide](Performance-Guide) for optimization strategies and the upcoming typed array roadmap.

---

## See Also

- [Feature Status](Feature-Status) — full implementation status matrix
- [Troubleshooting](Troubleshooting) — build and runtime failures
- [Performance Guide](Performance-Guide) — optimization strategies
