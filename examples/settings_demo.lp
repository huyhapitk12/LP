# @settings Decorator Demo
# This example demonstrates the @settings decorator for parallel and GPU execution

# ============================================
# Example 1: Basic parallel function with threads
# ============================================
@settings(threads=4)
def parallel_sum(n: int) -> int:
    """Calculate sum in parallel with 4 threads"""
    total = 0
    parallel for i in range(n):
        total += i
    return total

# ============================================
# Example 2: GPU-accelerated function
# ============================================
@settings(device="gpu", gpu_id=0)
def gpu_compute(data: list) -> list:
    """Run computation on GPU"""
    results = []
    parallel for item in data:
        results.append(item * 2)
    return results

# ============================================
# Example 3: Dynamic scheduling with chunk size
# ============================================
@settings(threads=8, schedule="dynamic", chunk=100)
def process_large_dataset(data: list) -> int:
    """Process large dataset with dynamic scheduling"""
    count = 0
    parallel for item in data:
        if item > 0:
            count += 1
    return count

# ============================================
# Example 4: Auto device selection
# ============================================
@settings(device="auto")
def auto_parallel_work(n: int) -> int:
    """Let the system choose the best device"""
    result = 0
    parallel for i in range(n):
        result += i * i
    return result

# ============================================
# Example 5: Combined @settings + @security
# ============================================
@settings(threads=4, device="cpu")
@security(level=2, auth=True)
def secure_parallel_process(data: list) -> list:
    """Parallel processing with security checks"""
    results = []
    parallel for item in data:
        results.append(item)
    return results

# ============================================
# Example 6: Guided scheduling for uneven workloads
# ============================================
@settings(schedule="guided", threads=6)
def process_uneven_workload(tasks: list) -> int:
    """Process tasks with guided scheduling for better load balancing"""
    completed = 0
    parallel for task in tasks:
        # Simulate varying work loads
        completed += 1
    return completed

# ============================================
# Example 7: Unified memory for GPU
# ============================================
@settings(device="gpu", unified=True)
def unified_memory_compute(size: int) -> list:
    """Use unified memory for seamless CPU/GPU data sharing"""
    results = []
    parallel for i in range(size):
        results.append(i)
    return results

# ============================================
# Main function to demonstrate
# ============================================
def main():
    print("=== @settings Decorator Demo ===")
    print()
    
    # Example 1
    print("1. Parallel sum (threads=4):")
    result = parallel_sum(1000)
    print(f"   Sum of 0..999 = {result}")
    print()
    
    # Example 3
    print("2. Process dataset (dynamic scheduling):")
    data = [1, -2, 3, -4, 5, -6, 7, 8, -9, 10]
    count = process_large_dataset(data)
    print(f"   Positive items: {count}")
    print()
    
    # Example 4
    print("3. Auto device selection:")
    result = auto_parallel_work(100)
    print(f"   Sum of squares: {result}")
    print()
    
    # Example 6
    print("4. Guided scheduling:")
    tasks = [1, 2, 3, 4, 5]
    completed = process_uneven_workload(tasks)
    print(f"   Completed: {completed} tasks")
    print()
    
    print("=== All examples completed! ===")

main()
