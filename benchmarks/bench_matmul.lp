import time

def main():
    n: int = 300
    sz: int = n * n
    a: list = [0] * sz
    b: list = [0] * sz
    c: list = [0] * sz
    for i in range(sz):
        a[i] = i % 100
        b[i] = (i * 2) % 100
    start: float = time.time()
    for i in range(n):
        for k in range(n):
            for j in range(n):
                c[i * n + j] = c[i * n + j] + a[i * n + k] * b[k * n + j]
    elapsed: float = time.time() - start
    print(c[n * n - 1])
    print(elapsed)

main()
