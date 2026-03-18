# Huong Dan LP Bang Tieng Viet

## Muc tieu

Tai lieu nay la ban onboarding tieng Viet de ban co the:

- build compiler LP
- chay chuong trinh dau tien
- hieu cu phap co ban
- dung cac module chinh
- dung CLI chinh
- hieu build host va cross-target
- xu ly cac loi pho bien

## Dieu kien truoc

- Ban da clone repo.
- Ban co C compiler phu hop voi may dang dung.

## Cai dat nhanh

### Windows

Duong build local duoc ho tro tot nhat hien nay:

- MSYS2 UCRT64 GCC

Build compiler:

```bat
build.bat
```

Kiem tra:

```bat
build\lp.exe --help
build\lp.exe test examples
```

### Linux hoac macOS

Build compiler:

```bash
make
# hoac
./compile.sh
```

Kiem tra:

```bash
./build/lp --help
./build/lp test examples
```

## Chuong trinh dau tien

Tao `hello.lp`:

```lp
def main():
    print("Xin chao tu LP")

main()
```

Chay (Native ASM - khong can GCC!):

```bash
lp hello.lp
```

Chay voi GCC backend:

```bash
lp hello.lp --gcc
```

Sinh C:

```bash
lp hello.lp -o hello.c
```

Compile ra executable:

```bash
lp hello.lp -c hello.exe
```

## Compilation Backends

### Native Assembly Backend (Mac dinh)

LP bien dich truc tiep sang assembly ma khong can GCC/LLVM:

```bash
lp file.lp              # Native compilation (default)
```

**Uu diem:**
- Khong can heavy compiler dependencies (~5MB thay vi ~1GB)
- Compilation nhanh
- Control truct tiep generated code

### GCC Backend (Tuy chon)

De su dung GCC backend:

```bash
lp file.lp --gcc        # Use GCC backend
```

## Optimizations

LP tu dong ap dung cac optimizations sau:

### Constant Folding

Evaluate constant expressions tai compile time:

```lp
# Truoc optimization
x: int = 1 + 2 * 3

# Sau optimization (compile-time)
x: int = 7
```

### Dead Code Elimination

Loai bo unreachable code:

```lp
if False:
    # Doan code nay bi loai bo
    print("Never executed")
```

### Loop Unrolling

Unroll small loops cho performance tot hon:

```lp
# Small constant loops duoc unroll
for i in range(4):
    print(i)
```

## Cu phap co ban

### Bien va kieu du lieu

```lp
n: int = 21
name: str = "LP"
ratio: float = 3.14
ok: bool = true
```

LP cung ho tro type inference:

```lp
n = 21
text = "LP"
```

### Ham

```lp
def twice(x: int) -> int:
    return x * 2
```

### Re nhanh va lap

```lp
if n > 10:
    print("lon")
else:
    print("nho")

for i in range(5):
    print(i)
```

### F-Strings

```lp
name = "LP"
version = 1
greeting = f"Hello from {name} v{version}!"
print(greeting)  # Output: Hello from LP v1!

# Bieu thuc trong f-string
x = 10
y = 20
print(f"Sum: {x + y}")  # Output: Sum: 30
```

### Pattern Matching (match/case)

```lp
match value:
    case 1:
        print("one")
    case 2:
        print("two")
    case _:
        print("other")

# Voi guard expression
match x:
    case n if n > 0:
        print("positive")
    case _:
        print("non-positive")
```

### Decorators

LP ho tro hai decorator chinh:

#### @settings - Cau hinh Parallel/GPU

```lp
# Cau hinh so threads
@settings(threads=8)
def parallel_process(data: list) -> list:
    results = []
    parallel for item in data:
        results.append(process(item))
    return results

# GPU acceleration
@settings(device="gpu", gpu_id=0)
def gpu_compute(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i * i
    return result

# Dynamic scheduling voi chunk size
@settings(threads=4, schedule="dynamic", chunk=100)
def process_large_dataset(data: list) -> int:
    count = 0
    parallel for item in data:
        count += item
    return count

# Auto-select best device
@settings(device="auto")
def auto_parallel(n: int) -> int:
    result = 0
    parallel for i in range(n):
        result += i
    return result
```

Cac tuy chon @settings:

| Setting | Type | Mo ta | Mac dinh |
|---------|------|-------|----------|
| `threads` | int | So threads (0 = auto) | 0 |
| `schedule` | string | "static", "dynamic", "guided", "auto" | "static" |
| `chunk` | int | Chunk size cho scheduling | 0 |
| `device` | string | "cpu", "gpu", "auto" | "cpu" |
| `gpu_id` | int | GPU device ID | 0 |
| `unified` | bool | Unified memory cho GPU | false |

#### @security - Bao mat

```lp
# Rate limiting va authentication
@security(level=3, auth=True, rate_limit=100)
def secure_api_endpoint(data: dict) -> dict:
    return {"status": "ok"}

# Access control
@security(admin=True)
def admin_function():
    pass

# Readonly mode
@security(readonly=True)
def read_only_function():
    pass
```

Cac tuy chon @security:

| Setting | Type | Mo ta |
|---------|------|-------|
| `level` | int | Muc bao mat (0-4) |
| `auth` | bool | Yeu cau authentication |
| `rate_limit` | int | So requests toi da/phut |
| `admin` | bool | Yeu cau admin access |
| `readonly` | bool | Chi doc |
| `validate` | bool | Validate input |
| `sanitize` | bool | Sanitize output |

### Class va ke thua

```lp
class Animal:
    def speak(self) -> str:
        return "animal"

class Dog(Animal):
    def speak(self) -> str:
        return "dog"
```

### `private` va `protected`

- `private`: chi dung duoc ben trong class khai bao no
- `protected`: dung duoc trong class va subclass
- LP check cac luat nay o compile-time

## Strings va collections

String methods dang duoc verify tot:

- `upper()`
- `lower()`
- `strip()`
- `find()`
- `count()`
- `split()`
- `join()`
- `startswith()`
- `endswith()`
- `isdigit()`

Collection literals:

```lp
nums = [1, 2, 3]
obj = {"name": "LP"}
pair = (1, 2)
```

Nested subscript kieu sau da duoc verify:

```lp
rows[0]["n"]
```

## Modules chinh

### HTTP

Ho tro:

- `http.get(url)`

Chua ho tro:

- `http.post(...)`

### JSON

Ho tro:

- `json.loads(text)`
- `json.dumps(value)`

Chua ho tro:

- `json.parse(...)`

### SQLite

Ho tro:

- `sqlite.connect(...)`
- `sqlite.execute(...)`
- `sqlite.query(...)`

### Thread

Ho tro:

- `thread.spawn(...)`
- `thread.join(...)`

Rang buoc hien tai cua `thread.spawn(...)`:

- worker phai la named LP function
- toi da 1 argument
- return type phai la `int` hoac `void`

### Memory

Ho tro:

- `memory.arena_new(...)`
- `memory.arena_alloc(...)`
- `memory.arena_reset(...)`
- `memory.arena_free(...)`
- `memory.pool_new(...)`
- `memory.pool_alloc(...)`
- `memory.pool_free(...)`
- `memory.pool_destroy(...)`
- `memory.cast(...)`

### Platform

Ho tro:

- `platform.os()`
- `platform.arch()`
- `platform.cores()`

## CLI chinh

```text
lp
lp file.lp
lp file.lp -o out.c
lp file.lp -c out.exe
lp file.lp -asm out.s
lp test examples
lp profile file.lp
lp watch file.lp
lp build file.lp --release --strip
lp package file.lp --format zip
lp export file.lp -o api_name
lp export file.lp --library -o api_name
```

## Build host va cross-target

Host build la build cho chinh may dang compile.

Cross-target la build cho OS hoac architecture khac:

```bash
lp build app.lp --target windows-x64
lp build app.lp --target linux-x64
lp build app.lp --target linux-arm64
lp build app.lp --target macos-arm64
```

Neu thieu toolchain, LP se bao loi ro rang som.

Toolchain mong doi hien tai:

- `windows-x64` -> `x86_64-w64-mingw32-gcc`
- `linux-x64` -> `x86_64-linux-gnu-gcc`
- `linux-arm64` -> `aarch64-linux-gnu-gcc`
- `macos-arm64` -> `aarch64-apple-darwin-gcc`

## Loi pho bien

### `Error: GCC not found`

Nguyen nhan:

- may chua co compiler phu hop

Cach sua:

- Windows: cai MSYS2 UCRT64 GCC
- Linux: cai `build-essential`
- macOS: cai Xcode Command Line Tools

### `Error: cannot find lp_runtime.h`

Nguyen nhan:

- LP khong tim thay runtime headers

Cach sua:

- chay compiler dung layout repo hoac dung layout cai dat dung

### `thread.spawn` bi reject

Nguyen nhan:

- worker khong phai named function
- worker co hon 1 tham so
- worker return type khong phai `int` hoac `void`

### `http.post` hoac `json.parse` bi reject

Nguyen nhan:

- day la API chua duoc ho tro trong compiler hien tai

## Tai lieu tiep theo

Neu muon doc day du hon, chuyen sang bo docs chinh bang tieng Anh:

- [Documentation Map]
- [Installation and Setup](Installation-and-Setup)
- [Runtime Modules](Runtime-Modules)
- [CLI and Tooling](CLI-and-Tooling)
- [Language Reference](Language-Reference)
