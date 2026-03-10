import thread

def bad_worker(a: int, b: int) -> int:
    return a + b

def main():
    t: thread = thread.spawn(bad_worker, 1)

main()