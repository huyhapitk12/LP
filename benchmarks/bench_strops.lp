import time

def main():
    s: str = "hello world foo bar baz qux"
    start: float = time.time()
    count: int = 0
    for i in range(200000):
        u: str = s.upper()
        l: str = u.lower()
        f: int = l.find("world")
        if f >= 0:
            count = count + 1
        count = count + 1
    elapsed: float = time.time() - start
    print(count)
    print(elapsed)

main()
