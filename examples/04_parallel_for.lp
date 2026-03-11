import math

def analyze_sample(sample_id: int) -> float:
    baseline: float = math.sin(sample_id * 0.01)
    smoothing: float = math.cos(sample_id * 0.015)
    return baseline * smoothing

def main():
    print("Processing one million independent samples...")

    # This pattern is good for work items that do not need a shared accumulator.
    # If you need reductions, aggregate explicitly after the loop.
    parallel for sample_id in range(1000000):
        analyze_sample(sample_id)

    print("Finished sample processing.")

main()
