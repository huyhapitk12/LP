import time

def main():
    n: int = 300
    W: int = 3000
    weights: list = [0] * (n + 1)
    values: list = [0] * (n + 1)
    for i in range(1, n + 1):
        weights[i] = (i * 7 + 3) % 50 + 1
        values[i] = (i * 13 + 5) % 100 + 1
    dp: list = [0] * (W + 1)
    start: float = time.time()
    for i in range(1, n + 1):
        wi: int = weights[i]
        vi: int = values[i]
        for w in range(W, wi - 1, -1):
            candidate: int = dp[w - wi] + vi
            if candidate > dp[w]:
                dp[w] = candidate
    elapsed: float = time.time() - start
    print(dp[W])
    print(elapsed)

main()
