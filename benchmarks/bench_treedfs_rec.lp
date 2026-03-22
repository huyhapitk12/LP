import time

def dfs(node: int, d: int, maxd: int) -> int:
    if d >= maxd:
        return 1
    return dfs(node * 2, d + 1, maxd) + dfs(node * 2 + 1, d + 1, maxd)

def main():
    start: float = time.time()
    result: int = dfs(1, 0, 20)
    elapsed: float = time.time() - start
    print(result)
    print(elapsed)
main()
