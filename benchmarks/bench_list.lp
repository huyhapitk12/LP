import time
def main():
    n: int = 200000
    vals: list = [0] * n
    nexts: list = [0] * n
    for i in range(n):
        vals[i] = i * 3 + 1
        nexts[i] = i + 1
    nexts[n - 1] = -1
    start: float = time.time()
    total: int = 0
    for pass_num in range(20):
        cur: int = 0
        while cur != -1:
            total = total + vals[cur]
            cur = nexts[cur]
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)
main()
