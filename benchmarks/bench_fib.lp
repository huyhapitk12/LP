import time

def fib(n: int) -> int:
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    start: float = time.time()
    result: int = fib(38)
    elapsed: float = time.time() - start
    print(result)
    print(elapsed)

main()
