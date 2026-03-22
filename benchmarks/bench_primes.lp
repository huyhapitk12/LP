import time

def count_factors(n: int) -> int:
    count: int = 0
    i: int = 2
    while i * i <= n:
        while n % i == 0:
            count = count + 1
            n = n // i
        i = i + 1
    if n > 1:
        count = count + 1
    return count

def main():
    start: float = time.time()
    total: int = 0
    for i in range(2, 100002):
        total = total + count_factors(i)
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)

main()
