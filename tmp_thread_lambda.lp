import thread

worker = lambda x:
    res: int = x + 1
    return res

def main():
    t: thread = thread.spawn(worker, 1)

main()