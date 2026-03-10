import thread

def bad_worker(a: int) -> str:
    return "bad"

def main():
    t: thread = thread.spawn(bad_worker, 1)

main()