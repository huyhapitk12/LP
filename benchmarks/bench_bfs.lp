import time

def main():
    rows: int = 1000
    cols: int = 1000
    total: int = rows * cols
    dist: list = [-1] * total
    q: list = [0] * total
    dist[0] = 0
    head: int = 0
    tail: int = 0
    q[tail] = 0
    tail = tail + 1
    dx: list = [0, 0, 1, -1]
    dy: list = [1, -1, 0, 0]
    start: float = time.time()
    while head < tail:
        cur: int = q[head]
        head = head + 1
        r: int = cur // cols
        c: int = cur % cols
        for d in range(4):
            nr: int = r + dx[d]
            nc: int = c + dy[d]
            if nr >= 0 and nr < rows and nc >= 0 and nc < cols:
                nb: int = nr * cols + nc
                if dist[nb] == -1:
                    dist[nb] = dist[cur] + 1
                    q[tail] = nb
                    tail = tail + 1
    elapsed: float = time.time() - start
    print(dist[total - 1])
    print(elapsed)

main()
