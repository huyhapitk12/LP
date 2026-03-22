import time
import math
def main():
    total: float = 0.0
    start: float = time.time()
    for i in range(5000000):
        total = total + math.sin(i * 0.001) + math.cos(i * 0.002) + math.sqrt(i + 1)
    elapsed: float = time.time() - start
    print(total)
    print(elapsed)
main()
