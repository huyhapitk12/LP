# Test file for new features: F-Strings, Pattern Matching, Decorators

# Test F-Strings
def test_fstrings():
    name = "LP"
    version = 1
    greeting = f"Hello from {name} v{version}!"
    print(greeting)

# Test Pattern Matching
def test_match():
    value = 2
    
    match value:
        case 1:
            print("one")
        case 2:
            print("two")
        case _:
            print("other")

# Test Decorators (simple)
@settings(threads=4)
def parallel_task():
    print("Running with 4 threads")

# Main
def main():
    test_fstrings()
    test_match()
    parallel_task()

main()
