import time

def main():
    maxd: int = 20
    start: float = time.time()

    # Iterative DFS using two arrays as stack
    node_stack: list = [0] * 2200000
    depth_stack: list = [0] * 2200000
    sp: int = 0
    node_stack[0] = 1
    depth_stack[0] = 0
    sp = 1
    count: int = 0

    while sp > 0:
        sp = sp - 1
        node: int = node_stack[sp]
        depth: int = depth_stack[sp]
        if depth >= maxd:
            count = count + 1
        else:
            node_stack[sp] = node * 2
            depth_stack[sp] = depth + 1
            sp = sp + 1
            node_stack[sp] = node * 2 + 1
            depth_stack[sp] = depth + 1
            sp = sp + 1

    elapsed: float = time.time() - start
    print(count)
    print(elapsed)

main()
