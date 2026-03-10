# Tài Liệu Học Tập Ngôn Ngữ Lập Trình LP (LP Language Study Guide)

Chào mừng bạn đến với tài liệu học tập ngôn ngữ LP. Ngôn ngữ LP kết hợp cú pháp dễ đọc của **Python** với tốc độ thực thi thần tốc của **C**. Mã nguồn LP được biên dịch (transpile) sang C tối ưu, và sau đó được biên dịch bằng phân hệ GCC `-O3`. Kết quả là hiệu năng thực thi có thể nhanh hơn Python từ 300 đến 700 lần.

Tài liệu này được chia thành các chương ngắn gọn, dễ hiểu để bạn có thể làm quen nhanh chóng với LP.

## Chương 1: Kiến thức Cơ bản (Basic)

### 1.1 Biến và Kiểu Dữ Liệu (Variables & Data Types)
LP sử dụng phong cách ghi chú kiểu dữ liệu (type hint) tương tự Python. LP có 4 kiểu dữ liệu cơ bản, tương ứng trực tiếp với các kiểu của ngôn ngữ C:
- `int`: Số nguyên 64-bit (`int64_t` trong C)
- `float`: Số thực 64-bit (`double` trong C)
- `str`: Chuỗi ký tự (`const char*` trong C)
- `bool`: Kiểu logic (`int` trong C, gán 1 hoặc 0)

**Ví dụ:**
```python
count: int = 42
pi: float = 3.14159
name: str = "LP Language"
is_fast: bool = True
```

### 1.2 Biên dịch và Chạy (Compile & Run)
Khi bạn lưu mã bằng một file `.lp` (ví dụ: `hello.lp`):
```bash
# Lệnh chạy trực tiếp một file (thông hiểu tức thì qua JIT / C backend):
lp hello.lp

# Lệnh biên dịch ra file mã nguồn C:
lp hello.lp -o hello.c

# Lệnh biên dịch trực tiếp ra file thực thi (.exe):
lp hello.lp -c hello.exe
```

---

## Chương 2: Quá trình điều khiển (Control Flow)

### 2.1 Câu lệnh Điều kiện (If / Elif / Else)
LP sử dụng thụt lề (indentation) như Python:
```python
x: int = 10
if x > 0:
    print("Positive")
elif x == 0:
    print("Zero")
else:
    print("Negative")
```

### 2.2 Vòng lặp For và While
Vòng lặp trong LP được dịch trực tiếp thành các vòng lặp C gốc cực nhanh.
```python
# Vòng lặp For với range(start, end)
for i in range(5, 10):
    print(i) # In từ 5 đến 9

# Vòng lặp While
n: int = 5
while n > 0:
    print(n)
    n = n - 1
```

---

## Chương 3: Hàm (Functions)

Bạn định nghĩa hàm bằng từ khóa `def`, kết hợp kiểu dữ liệu của tham số và đầu ra (nếu không khai báo trả về, mặc định là kiểu `float`).

```python
def add(a: int, b: int) -> int:
    return a + b

# Tham số có độ dài linh hoạt (*args)
def dynamic_sum(*args) -> float:
    total: float = 0.0
    for i in range(len(args)):
        total = total + args[i]
    return total

# Tham số dạng từ khóa (**kwargs)
def print_settings(**settings):
    print(settings)

# Lambda Functions (Hàm ẩn danh)
square = lambda x: x * x
print(square(10)) # 100
```

---

## Chương 4: Cấu trúc Dữ liệu (Data Structures)

Ngôn ngữ LP hỗ trợ các cấu trúc dữ liệu quen thuộc như Python:
*   **List (Mảng C được Dynamic hóa)**: `numbers = [1, 2, 3, 4, 5]`
*   **List Comprehension**: Khai báo danh sách gọn gàng sử dụng tính năng nội suy.
    `squares = [x * x for x in 10]`
*   **Dictionaries / Maps**: `person = {"name": "Alice", "age": 30}`
*   **Sets**: `unique = {1, 2, 3, 2, 1}` (tự động loại bỏ phần tử trùng)
*   **Tuples**: `point = (10, 20)`

---

## Chương 5: Xử lý Ngoại lệ (Exceptions) và Đọc Ghi File I/O

### 5.1 Try / Except / Finally
```python
try:
    result: int = 10 / 0
except:
    print("Lỗi chia cho 0!")
finally:
    print("Thực thi đoạn mã don dẹp")
```
_Lưu ý: Nếu có lỗi logic nghiêm trọng, có thể sử dụng từ khóa `raise` để ném lỗi tùy chỉnh: `raise "Error Message"`_

### 5.2 Context Manager cho File I/O
Đọc và ghi file dễ dàng với cấu trúc khối `with`:
```python
with open("data.txt", "w") as f:
    f.write("Xin chào từ LP!")

with open("data.txt", "r") as f:
    noidung: str = f.read()
    print(noidung)
```

---

## Chương 6: Lập trình Hướng đối tượng (OOP) và Modules

LP hỗ trợ khai báo Class (Lớp), thuộc tính và Phương thức (`self`):
```python
class Animal:
    name = "Cat"
    age = 2

    def sound(self):
        print("Meow!")

cat = Animal()
cat.sound()
```

Bên cạnh đó, LP còn tích hợp sẵn một hệ thống `import` để mang các thư viện chuẩn vào không gian làm việc như `math`, `time`, `random`.
```python
import math
import time

start: float = time.time()
print(math.sqrt(144)) # In ra: 12.0
print("Thời gian chạy (giây):", time.time() - start)
```

---

## Mẹo Cải thiện Hiệu năng (Performance Tips)

1. **Dùng kiểu tĩnh (Static Typing)**: Khai báo kiểu tường minh (ví dụ: `x: int = 0`) giúp trình biên dịch C bỏ qua bộ kiểm tra ép kiểu động.
2. **Hạn chế phép toán số thực (float) cho vòng lặp**: Sử dụng `int` làm bộ đếm chuẩn.
3. **Sử dụng thư viện tích hợp (Built-in Standard Library)**: Việc gọi `import math` của LP thường sẽ được chuyển mã trực tiếp ra hàm tương ứng trong chuẩn C (`math.h`) thay vì gọi vào tầng Python, đẩy nhanh tốc độ lên tột đỉnh.

Chúc bạn học tốt và có thể ứng dụng sức mạnh của ngôn ngữ LP để tăng tốc các dự án Python của mình!
