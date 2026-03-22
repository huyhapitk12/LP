import time

def main():
    n: int = 1000000
    arr: list = [0] * n
    for i in range(n):
        arr[i] = i * 2

    start: float = time.time()
    found: int = 0
    for qi in range(1000000):
        target: int = (qi * 7) % (n * 2)
        lo: int = 0
        hi: int = n - 1
        while lo <= hi:
            mid: int = (lo + hi) // 2
            if arr[mid] == target:
                found = found + 1
                lo = hi + 1
            elif arr[mid] < target:
                lo = mid + 1
            else:
                hi = mid - 1
    elapsed: float = time.time() - start
    print(found)
    print(elapsed)

main()
