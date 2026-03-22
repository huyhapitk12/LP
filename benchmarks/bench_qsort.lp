import time

def main():
    n: int = 100000
    arr: list = [0] * n
    for i in range(n):
        arr[i] = (i * 1103515245 + 12345) % 1000000

    start: float = time.time()

    stack_lo: list = [0] * 40
    stack_hi: list = [0] * 40
    top: int = 0
    stack_lo[0] = 0
    stack_hi[0] = n - 1

    while top >= 0:
        lo: int = stack_lo[top]
        hi: int = stack_hi[top]
        top = top - 1

        if lo < hi:
            pivot: int = arr[hi]
            i: int = lo - 1
            for j in range(lo, hi):
                if arr[j] <= pivot:
                    i = i + 1
                    tmp: int = arr[i]
                    arr[i] = arr[j]
                    arr[j] = tmp
            i = i + 1
            tmp: int = arr[i]
            arr[i] = arr[hi]
            arr[hi] = tmp
            p: int = i

            if p - 1 > lo:
                top = top + 1
                stack_lo[top] = lo
                stack_hi[top] = p - 1
            if p + 1 < hi:
                top = top + 1
                stack_lo[top] = p + 1
                stack_hi[top] = hi

    elapsed: float = time.time() - start
    print(arr[n//2])
    print(elapsed)

main()
