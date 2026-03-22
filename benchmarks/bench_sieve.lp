import time

def main():
    n: int = 5000000
    sieve: list = [1] * (n + 1)
    sieve[0] = 0
    sieve[1] = 0
    start: float = time.time()
    i: int = 2
    while i * i <= n:
        if sieve[i] == 1:
            j: int = i * i
            while j <= n:
                sieve[j] = 0
                j = j + i
        i = i + 1
    count: int = 0
    for k in range(n + 1):
        if sieve[k] == 1:
            count = count + 1
    elapsed: float = time.time() - start
    print(count)
    print(elapsed)

main()
