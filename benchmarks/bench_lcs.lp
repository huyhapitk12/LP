import time

def main():
    n: int = 1000
    a: list = [0] * n
    b: list = [0] * n
    for i in range(n):
        a[i] = (i * 1103515245 + 12345) % 26
        b[i] = (i * 22695477 + 1) % 26
    dp: list = [0] * ((n+1) * (n+1))
    start: float = time.time()
    for i in range(1, n+1):
        for j in range(1, n+1):
            if a[i-1] == b[j-1]:
                dp[i*(n+1)+j] = dp[(i-1)*(n+1)+(j-1)] + 1
            elif dp[(i-1)*(n+1)+j] >= dp[i*(n+1)+(j-1)]:
                dp[i*(n+1)+j] = dp[(i-1)*(n+1)+j]
            else:
                dp[i*(n+1)+j] = dp[i*(n+1)+(j-1)]
    elapsed: float = time.time() - start
    print(dp[n*(n+1)+n])
    print(elapsed)
main()
