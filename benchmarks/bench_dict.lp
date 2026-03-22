import time

def main():
    start: float = time.time()
    d: dict = {}
    for i in range(100000):
        key: str = str(i % 10000)
        val = d[key]
        if val == None:
            d[key] = 1
        else:
            d[key] = val + 1
    total: int = 0
    for i in range(10000):
        k: str = str(i)
        v = d[k]
        if v != None:
            total = total + v
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)

main()
