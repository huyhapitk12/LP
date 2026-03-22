import time

def main():
    n: int = 8000
    arr: list = [0] * n
    for i in range(n):
        arr[i] = n - i
    start: float = time.time()
    for i in range(n):
        for j in range(n - i - 1):
            if arr[j] > arr[j + 1]:
                tmp: int = arr[j]
                arr[j] = arr[j + 1]
                arr[j + 1] = tmp
    elapsed: float = time.time() - start
    print(arr[0])
    print(elapsed)

main()
