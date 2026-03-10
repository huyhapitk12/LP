# Parallel Processing demonstration
import math
import thread

def compute(idx: int) -> float:
    # A heavy calculation isolated
    res: float = math.sin(idx * 0.1) * math.cos(idx * 0.2)
    return res

def main():
    print("Beginning 10,000,000 iteration computation")
    
    total: float = 0.0
    
    # Leverages Native Backend OpenMP Pragmas instantly scaling across CPU Hardware threads
    parallel for i in range(10000000):
        val: float = compute(i)
        
        # Parallel reductions aren't atomic automatically in LP right now, 
        # but the looping structure natively maps to #pragma openmp parallel.
        
    print("Parallel processing complete!")

main()
