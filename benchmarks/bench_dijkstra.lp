import time

def iabs(x: int) -> int:
    if x < 0:
        return -x
    return x

def main():
    n: int = 500
    INF: int = 1000000000
    adj: list = [INF] * (n * n)
    for i in range(n):
        adj[i * n + i] = 0
    for i in range(n):
        for j in range(n):
            diff: int = iabs(i - j)
            if diff > 0 and diff <= 2:
                adj[i * n + j] = (i * 7 + j * 13) % 100 + 1

    dist: list = [INF] * n
    visited: list = [0] * n
    dist[0] = 0

    start: float = time.time()
    for step in range(n):
        u: int = -1
        for v in range(n):
            if visited[v] == 0:
                if u == -1 or dist[v] < dist[u]:
                    u = v
        if u == -1 or dist[u] == INF:
            break
        visited[u] = 1
        for v in range(n):
            w: int = adj[u * n + v]
            if w < INF and dist[u] + w < dist[v]:
                dist[v] = dist[u] + w
    elapsed: float = time.time() - start
    print(dist[n - 1])
    print(elapsed)

main()
