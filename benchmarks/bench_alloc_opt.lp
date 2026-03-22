import time

def main():
    start: float = time.time()
    total: int = 0
    arr: list = [0] * 1000
    for i in range(10000):
        for j in range(1000):
            arr[j] = j * i
        total = total + arr[999]
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)
main()
