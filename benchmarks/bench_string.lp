import time
def main():
    s: str = "abcdefghijklmnopqrstuvwxyz"
    total: int = 0
    start: float = time.time()
    for i in range(500000):
        upper_s: str = s.upper()
        lower_s: str = upper_s.lower()
        found: int = lower_s.find("xyz")
        total = total + found
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)
main()
