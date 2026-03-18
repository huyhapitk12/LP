# LP Language - Báo cáo Khả năng Lập trình Thi đấu (Competitive Programming)

## Tổng quan

Báo cáo này phân tích khả năng sử dụng LP Language cho lập trình thi đấu (CP), so sánh với các ngôn ngữ phổ biến như C++, Python, Java.

---

## 1. I/O Operations (Nhập/Xuất)

### ✅ Có sẵn

| Feature | LP Syntax | Mô tả |
|---------|-----------|-------|
| `print()` | `print(x)` | Xuất ra stdout |
| `input()` | `input(prompt)` | Đọc dòng từ stdin |
| File I/O | `open(path, mode)` | Đọc/ghi file |

### ⚠️ Hạn chế cho CP

| Feature | Trạng thái | Ghi chú |
|---------|------------|---------|
| Fast I/O | ❌ Không có | Không có `scanf`/`printf` nhanh như C |
| Multi-value input | ⚠️ Cần parse | `input()` trả về string, cần split + int() |
| Buffer I/O | ❌ Không có | Không có `sys.stdin.buffer` như Python |

### Ví dụ đọc nhiều số trên 1 dòng

```lp
# LP Code - đọc n số trên 1 dòng
line: str = input()
parts = line.split(" ")
n = int(parts[0])
m = int(parts[1])
```

**So sánh với C++:**
```cpp
// C++ - nhanh và gọn hơn
int n, m;
cin >> n >> m;
```

**So sánh với Python:**
```python
# Python - tương tự LP
n, m = map(int, input().split())
```

---

## 2. Cấu trúc Dữ liệu

### ✅ Có sẵn

| Cấu trúc | LP Syntax | Độ phức tạp |
|----------|-----------|-------------|
| **Array/List** | `[1, 2, 3]` | O(1) access, O(n) append |
| **Dict/Map** | `{"key": value}` | O(1) average lookup |
| **Set** | `{1, 2, 3}` | O(1) average lookup |
| **Tuple** | `(1, 2)` | O(1) access |
| **NumPy Array** | `np.array([1,2,3])` | O(1) access, SIMD optimized |

### ❌ Thiếu cho CP

| Cấu trúc | Trạng thái | Impact |
|----------|------------|--------|
| **Stack** | ❌ Không có builtin | Cần tự implement với list |
| **Queue** | ❌ Không có builtin | Cần tự implement |
| **Priority Queue/Heap** | ❌ Không có | Thiếu cho Dijkstra, BFS ưu tiên |
| **Deque** | ❌ Không có | Thiếu cho sliding window |
| **Disjoint Set (DSU)** | ❌ Không có | Thiếu cho MST |
| **Segment Tree** | ❌ Không có | Thiếu cho range queries |
| **Fenwick Tree (BIT)** | ❌ Không có | Thiếu cho range sum |

### Ví dụ implement Stack với List

```lp
# Stack implementation
stack = []
stack.append(1)     # push
stack.append(2)
top = stack[len(stack)-1]  # peek -> 2
stack.pop()         # pop (cần tự implement pop_last)
```

---

## 3. Thuật toán

### ✅ Có sẵn trong NumPy

| Thuật toán | Function | Ghi chú |
|------------|----------|---------|
| Sort | `np.sort(arr)` | O(n log n) |
| Binary Search | `np.searchsorted()` | O(log n) |
| Min/Max | `np.min(arr)`, `np.max(arr)` | O(n) |
| Sum | `np.sum(arr)` | O(n) |
| Dot Product | `np.dot(a, b)` | O(n) |
| Matrix Mul | `np.matmul(a, b)` | O(n³) |

### ✅ Có sẵn trong List

| Thuật toán | Function | Ghi chú |
|------------|----------|---------|
| Sort | `lp_list_sort(list)` | qsort O(n log n) |
| Binary Search | `lp_list_binary_search(list, val)` | O(log n) |

### ❌ Thiếu cho CP

| Thuật toán | Trạng thái | Use Case |
|------------|------------|----------|
| **BFS/DFS** | ❌ Không có builtin | Graph traversal |
| **Dijkstra** | ❌ Không có | Shortest path |
| **Floyd-Warshall** | ❌ Không có | All-pairs shortest |
| **Binary Search (int)** | ⚠️ Cần tự implement | Tìm kiếm nhị phân |
| **Two Pointers** | ✅ Có thể implement | Sliding window |
| **DP templates** | ❌ Không có | Cần tự code |
| **Union Find** | ❌ Không có | DSU |
| **Topological Sort** | ❌ Không có | DAG |

---

## 4. Math Functions

### ✅ Có đầy đủ

| Function | LP Syntax | Mô tả |
|----------|-----------|-------|
| sqrt | `math.sqrt(x)` | Căn bậc 2 |
| pow | `math.pow(x, y)` | Lũy thừa |
| abs | `math.fabs(x)` | Giá trị tuyệt đối |
| ceil | `math.ceil(x)` | Làm tròn lên |
| floor | `math.floor(x)` | Làm tròn xuống |
| round | `math.round(x)` | Làm tròn |
| **gcd** | `math.gcd(a, b)` | ✅ UCLN |
| **lcm** | `math.lcm(a, b)` | ✅ BCNN |
| **factorial** | `math.factorial(n)` | ✅ Giai thừa |
| sin/cos/tan | `math.sin(x)` | Lượng giác |
| log/exp | `math.log(x)`, `math.exp(x)` | Logarith/Exponential |
| Constants | `math.pi`, `math.e` | Hằng số |

### Ví dụ tính GCD/LCM

```lp
import math

def solve():
    a = 12
    b = 18
    g = math.gcd(a, b)    # 6
    l = math.lcm(a, b)    # 36
    print(g)
    print(l)
```

---

## 5. Bit Manipulation

### ✅ Có đầy đủ

| Operator | LP Syntax | Mô tả |
|----------|-----------|-------|
| AND | `a & b` | AND bitwise |
| OR | `a \| b` | OR bitwise |
| XOR | `a ^ b` | XOR bitwise |
| NOT | `~a` | NOT bitwise |
| Left Shift | `a << n` | Dịch trái |
| Right Shift | `a >> n` | Dịch phải |
| Compound | `a &= b`, `a \|= b` | Gán kết hợp |

### Ví dụ Bit Manipulation

```lp
# Kiểm tra bit thứ i
def is_set(n: int, i: int) -> int:
    return (n >> i) & 1

# Bật bit thứ i
def set_bit(n: int, i: int) -> int:
    return n | (1 << i)

# Tắt bit thứ i
def clear_bit(n: int, i: int) -> int:
    return n & ~(1 << i)

# Đếm số bit 1
def count_bits(n: int) -> int:
    count = 0
    while n > 0:
        count = count + (n & 1)
        n = n >> 1
    return count
```

---

## 6. So sánh với các ngôn ngữ CP phổ biến

### So sánh tổng quan

| Feature | LP | C++ | Python | Java |
|---------|----|----|--------|------|
| **Compilation** | Native | Native | Interpreted | Bytecode |
| **Speed** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| **I/O Speed** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐ |
| **STL/Data Structures** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Math Library** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Syntax Simplicity** | ⭐⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐ |
| **Debugging** | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **CP Support** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |

### Điểm mạnh của LP cho CP

1. **Cú pháp giống Python** - Dễ học, dễ code nhanh
2. **Compile to Native** - Tốc độ gần C++
3. **NumPy builtin** - Tính toán ma trận nhanh
4. **GCD/LCM/Factorial builtin** - Thường dùng trong CP
5. **Bitwise operators đầy đủ** - bitmask problems

### Điểm yếu của LP cho CP

1. **Thiếu Stack/Queue/Heap builtin** - Phải tự implement
2. **Thiếu Graph algorithms** - Không có BFS/DFS/Dijkstra
3. **I/O chưa tối ưu** - Không có fast I/O như C++
4. **Thiếu DSU, Segment Tree** - Phức tạp cho advanced problems
5. **Không có online judge support** - Không được support trên Codeforces, AtCoder...

---

## 7. Ví dụ bài toán CP trong LP

### Bài 1: Two Sum (Easy)

```lp
def two_sum(nums: list, target: int) -> list:
    for i in range(len(nums)):
        for j in range(i + 1, len(nums)):
            if nums[i] + nums[j] == target:
                return [i, j]
    return [-1, -1]

# Test
arr = [2, 7, 11, 15]
result = two_sum(arr, 9)
print(result)  # [0, 1]
```

### Bài 2: GCD của dãy số

```lp
import math

def gcd_array(arr: list) -> int:
    result = arr[0]
    for i in range(1, len(arr)):
        result = math.gcd(result, arr[i])
    return result

# Test
arr = [12, 24, 36, 48]
print(gcd_array(arr))  # 12
```

### Bài 3: Binary Search

```lp
def binary_search(arr: list, target: int) -> int:
    left = 0
    right = len(arr) - 1
    
    while left <= right:
        mid = (left + right) // 2
        if arr[mid] == target:
            return mid
        elif arr[mid] < target:
            left = mid + 1
        else:
            right = mid - 1
    
    return -1

# Test
arr = [1, 3, 5, 7, 9, 11]
print(binary_search(arr, 7))  # 3
```

### Bài 4: Fibonacci với DP

```lp
def fib(n: int) -> int:
    if n <= 1:
        return n
    
    dp = [0] * (n + 1)
    dp[1] = 1
    
    for i in range(2, n + 1):
        dp[i] = dp[i-1] + dp[i-2]
    
    return dp[n]

# Test
print(fib(10))  # 55
```

### Bài 5: Sieve of Eratosthenes

```lp
def sieve(n: int) -> list:
    is_prime = [True] * (n + 1)
    is_prime[0] = False
    is_prime[1] = False
    
    i = 2
    while i * i <= n:
        if is_prime[i]:
            j = i * i
            while j <= n:
                is_prime[j] = False
                j = j + i
        i = i + 1
    
    primes = []
    for i in range(2, n + 1):
        if is_prime[i]:
            primes.append(i)
    
    return primes

# Test
primes = sieve(30)
print(primes)  # [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
```

---

## 8. Kết luận & Đề xuất

### Đánh giá tổng thể: ⭐⭐⭐ (3/5) cho CP

### Khi nào nên dùng LP cho CP?

✅ **Nên dùng khi:**
- Bài toán đơn giản, không cần cấu trúc dữ liệu phức tạp
- Bài toán toán học (GCD, LCM, factorial)
- Bài toán bitmask
- Bài toán array/matrix operations (NumPy)
- Học thuật, làm quen với lập trình

❌ **Không nên dùng khi:**
- Bài toán graph (BFS/DFS/Dijkstra)
- Bài toán cần Segment Tree, Fenwick Tree
- Bài toán cần Heap/Priority Queue
- Contest có time limit chặt
- Submit lên Online Judge (không được support)

### Đề xuất cải thiện cho CP

1. **Thêm Fast I/O module:**
   ```lp
   import fastio
   n = fastio.read_int()
   arr = fastio.read_ints()  # đọc nhiều int trên 1 dòng
   fastio.write(n)
   ```

2. **Thêm collections module:**
   ```lp
   import collections
   q = collections.Queue()
   s = collections.Stack()
   pq = collections.PriorityQueue()
   ```

3. **Thêm algorithms module:**
   ```lp
   import algo
   algo.bfs(graph, start)
   algo.dfs(graph, start)
   algo.dijkstra(graph, src)
   ```

4. **Thêm DSU:**
   ```lp
   dsu = DSU(n)
   dsu.union(a, b)
   dsu.find(x)
   ```

---

## 9. Tham chiếu nhanh LP cho CP

### Template cơ bản

```lp
import math

def solve():
    # Đọc input
    line = input()
    parts = line.split(" ")
    n = int(parts[0])
    
    # Xử lý
    result = n * 2
    
    # Xuất output
    print(result)

solve()
```

### Các hàm thường dùng

```lp
# Math
math.gcd(a, b)      # UCLN
math.lcm(a, b)      # BCNN
math.factorial(n)   # n!
math.sqrt(x)        # √x
math.pow(x, y)      # x^y
math.floor(x)       # ⌊x⌋
math.ceil(x)        # ⌈x⌉

# Bitwise
a & b               # AND
a | b               # OR
a ^ b               # XOR
~a                  # NOT
a << n              # Left shift
a >> n              # Right shift

# List
len(arr)            # Độ dài
arr.append(x)       # Thêm phần tử
arr[i]              # Truy cập
```

---

*Báo cáo được tạo tự động từ phân tích source code LP Language*
*Ngày: $(date)*
