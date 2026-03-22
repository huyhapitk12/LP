import time

def main():
    start: float = time.time()
    total: int = 0
    for i in range(100000000):
        total = total + i
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)

main()
