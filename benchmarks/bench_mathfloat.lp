import time
import math

def main():
    start: float = time.time()
    total: float = 0.0
    for i in range(1000000):
        x: float = (i + 1) * 0.000001
        total = total + math.sin(x) * math.cos(x) + math.sqrt(x) * math.log(x + 1.0)
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)
main()
