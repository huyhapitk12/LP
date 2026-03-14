# Parallel Computing Test in LP Language
# Demonstrates OpenMP parallel for with settings

import math

# Simple parallel for loop
def parallel_sum(n: int) -> int:
    result: int = 0
    parallel for i in range(n):
        result += i
    return result

# Parallel for with range and step
def parallel_process(arr_size: int):
    parallel for i in range(0, arr_size, 2):
        print(i)

# Main
def main() -> int:
    print("Testing parallel computing in LP")
    
    # Test basic parallel for
    total: int = parallel_sum(1000)
    print("Sum from 0 to 999:")
    print(total)
    
    # Test parallel with step
    parallel_process(20)
    
    return 0
